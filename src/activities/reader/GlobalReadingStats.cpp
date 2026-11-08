#include "GlobalReadingStats.h"

#include <ByteOrder.h>
#include <HalStorage.h>
#include <Logging.h>
#include <esp_mac.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string>

using byteorder::readLe16;
using byteorder::readLe32;

namespace {
enum class StatsLoadResult : uint8_t { Ok, Invalid, NewerFormat };

struct StatsLoadOutcome {
  StatsLoadResult result = StatsLoadResult::Invalid;
  uint8_t version = 0;
  size_t fileSize = 0;
};

// Binary layout v1 (13 bytes):
//   [0]     version (= 1)
//   [1-4]   totalSessions       uint32_t LE
//   [5-8]   totalReadingSeconds uint32_t LE
//   [9-12]  totalPagesTurned    uint32_t LE
//
// Binary layout v2 (17 bytes):
//   [0]      version (= 2)
//   [1-4]    totalSessions       uint32_t LE
//   [5-8]    totalReadingSeconds uint32_t LE
//   [9-12]   totalPagesTurned    uint32_t LE
//   [13-16]  completedBooks      uint32_t LE
//
// Binary layout v3 (159 bytes):
//   [0]       version (= 3)
//   [1-4]     totalSessions             uint32_t LE
//   [5-8]     totalReadingSeconds       uint32_t LE
//   [9-12]    totalPagesTurned          uint32_t LE
//   [13-16]   completedBooks            uint32_t LE
//   [17-32]   timeOfDaySeconds[4]       uint32_t LE each
//   [33-60]   dayOfWeekSeconds[7]       uint32_t LE each
//   [61-64]   readingHistoryAnchorDay   uint32_t LE
//   [65-156]  readingHistoryBits[92]    uint8_t
//   [157-158] longestReadingStreak      uint16_t LE
static constexpr uint8_t GLOBAL_STATS_VERSION = GlobalReadingStats::CURRENT_FILE_VERSION;
static constexpr uint8_t GLOBAL_STATS_VERSION_V1 = 1;
static constexpr int GLOBAL_STATS_FILE_SIZE_V1 = 13;
static constexpr uint8_t GLOBAL_STATS_VERSION_V2 = 2;
static constexpr int GLOBAL_STATS_FILE_SIZE_V2 = 17;
static constexpr int GLOBAL_STATS_FILE_SIZE = static_cast<int>(GlobalReadingStats::CURRENT_FILE_SIZE);
static constexpr char GLOBAL_STATS_PATH[] = "/.crosspoint/global_stats.bin";
static constexpr char GLOBAL_STATS_BAK_PATH[] = "/.crosspoint/global_stats.bin.bak";
static constexpr char SYNCED_STATS_DIR[] = "/.crosspoint/synced_stats";
static constexpr char DAILY_READING_PATH[] = "/.crosspoint/daily_reading.bin";

// daily_reading.bin: dailyAnchorDay (uint32_t LE), then READING_DAILY_DAYS uint16_t LE seconds, newest first.
static void loadDailyReading(GlobalReadingStats& stats) {
  FsFile f;
  if (!Storage.openFileForRead("GSTATS", DAILY_READING_PATH, f)) return;
  uint8_t data[4 + 2 * READING_DAILY_DAYS];
  if (f.read(data, sizeof(data)) == static_cast<int>(sizeof(data))) {
    stats.dailyAnchorDay = data[0] | data[1] << 8 | data[2] << 16 | static_cast<uint32_t>(data[3]) << 24;
    for (size_t i = 0; i < READING_DAILY_DAYS; ++i) stats.dailySeconds[i] = data[4 + 2 * i] | data[5 + 2 * i] << 8;
  }
  f.close();
}

static void saveDailyReading(const GlobalReadingStats& stats) {
  uint8_t data[4 + 2 * READING_DAILY_DAYS];
  for (int b = 0; b < 4; ++b) data[b] = static_cast<uint8_t>(stats.dailyAnchorDay >> (8 * b));
  for (size_t i = 0; i < READING_DAILY_DAYS; ++i) {
    data[4 + 2 * i] = static_cast<uint8_t>(stats.dailySeconds[i]);
    data[5 + 2 * i] = static_cast<uint8_t>(stats.dailySeconds[i] >> 8);
  }
  FsFile f;
  if (!Storage.openFileForWrite("GSTATS", DAILY_READING_PATH, f)) return;
  f.write(data, sizeof(data));
  f.close();
}

static void addDailySeconds(GlobalReadingStats& stats, const uint32_t day, const uint32_t seconds) {
  auto& days = stats.dailySeconds;
  if (day > stats.dailyAnchorDay) {
    const uint32_t shift = day - stats.dailyAnchorDay;
    for (size_t i = days.size(); i-- > 0;) days[i] = i >= shift ? days[i - shift] : 0;
    stats.dailyAnchorDay = day;
  }
  const uint32_t age = stats.dailyAnchorDay - day;
  if (age < days.size()) days[age] = static_cast<uint16_t>(std::min<uint32_t>(UINT16_MAX, days[age] + seconds));
}
static bool s_blockDestructiveSave = false;

void loadCommonFields(const uint8_t* data, GlobalReadingStats& out) {
  out.totalSessions = readLe32(data, 1);
  out.totalReadingSeconds = readLe32(data, 5);
  out.totalPagesTurned = readLe32(data, 9);
}

uint32_t addSaturated(const uint32_t a, const uint32_t b) {
  const uint32_t max = std::numeric_limits<uint32_t>::max();
  return max - a < b ? max : a + b;
}

void addStats(GlobalReadingStats& target, const GlobalReadingStats& source) {
  target.totalSessions = addSaturated(target.totalSessions, source.totalSessions);
  target.totalReadingSeconds = addSaturated(target.totalReadingSeconds, source.totalReadingSeconds);
  target.totalPagesTurned = addSaturated(target.totalPagesTurned, source.totalPagesTurned);
  target.completedBooks = addSaturated(target.completedBooks, source.completedBooks);
  for (size_t i = 0; i < target.timeOfDaySeconds.size(); ++i) {
    target.timeOfDaySeconds[i] = addSaturated(target.timeOfDaySeconds[i], source.timeOfDaySeconds[i]);
  }
  for (size_t i = 0; i < target.dayOfWeekSeconds.size(); ++i) {
    target.dayOfWeekSeconds[i] = addSaturated(target.dayOfWeekSeconds[i], source.dayOfWeekSeconds[i]);
  }
  mergeReadingHistory(target.readingHistoryAnchorDay, target.readingHistoryBits, source.readingHistoryAnchorDay,
                      source.readingHistoryBits);
  target.longestReadingStreak = std::max(target.longestReadingStreak, source.longestReadingStreak);
}

void serializeStats(const GlobalReadingStats& stats, uint8_t* data) {
  data[0] = GLOBAL_STATS_VERSION;
  data[1] = stats.totalSessions & 0xFF;
  data[2] = (stats.totalSessions >> 8) & 0xFF;
  data[3] = (stats.totalSessions >> 16) & 0xFF;
  data[4] = (stats.totalSessions >> 24) & 0xFF;
  data[5] = stats.totalReadingSeconds & 0xFF;
  data[6] = (stats.totalReadingSeconds >> 8) & 0xFF;
  data[7] = (stats.totalReadingSeconds >> 16) & 0xFF;
  data[8] = (stats.totalReadingSeconds >> 24) & 0xFF;
  data[9] = stats.totalPagesTurned & 0xFF;
  data[10] = (stats.totalPagesTurned >> 8) & 0xFF;
  data[11] = (stats.totalPagesTurned >> 16) & 0xFF;
  data[12] = (stats.totalPagesTurned >> 24) & 0xFF;
  data[13] = stats.completedBooks & 0xFF;
  data[14] = (stats.completedBooks >> 8) & 0xFF;
  data[15] = (stats.completedBooks >> 16) & 0xFF;
  data[16] = (stats.completedBooks >> 24) & 0xFF;
  for (size_t i = 0; i < stats.timeOfDaySeconds.size(); ++i) {
    const uint32_t value = stats.timeOfDaySeconds[i];
    const int offset = 17 + static_cast<int>(i) * 4;
    data[offset] = value & 0xFF;
    data[offset + 1] = (value >> 8) & 0xFF;
    data[offset + 2] = (value >> 16) & 0xFF;
    data[offset + 3] = (value >> 24) & 0xFF;
  }
  for (size_t i = 0; i < stats.dayOfWeekSeconds.size(); ++i) {
    const uint32_t value = stats.dayOfWeekSeconds[i];
    const int offset = 33 + static_cast<int>(i) * 4;
    data[offset] = value & 0xFF;
    data[offset + 1] = (value >> 8) & 0xFF;
    data[offset + 2] = (value >> 16) & 0xFF;
    data[offset + 3] = (value >> 24) & 0xFF;
  }
  data[61] = stats.readingHistoryAnchorDay & 0xFF;
  data[62] = (stats.readingHistoryAnchorDay >> 8) & 0xFF;
  data[63] = (stats.readingHistoryAnchorDay >> 16) & 0xFF;
  data[64] = (stats.readingHistoryAnchorDay >> 24) & 0xFF;
  memcpy(data + 65, stats.readingHistoryBits.data(), stats.readingHistoryBits.size());
  data[157] = stats.longestReadingStreak & 0xFF;
  data[158] = (stats.longestReadingStreak >> 8) & 0xFF;
}

StatsLoadOutcome loadFromOpenFile(FsFile& f, GlobalReadingStats& out) {
  StatsLoadOutcome outcome;
  outcome.fileSize = f.fileSize();

  uint8_t data[GLOBAL_STATS_FILE_SIZE] = {};
  const size_t bytesToRead = std::min(outcome.fileSize, static_cast<size_t>(GLOBAL_STATS_FILE_SIZE));
  const int n = f.read(data, bytesToRead);
  if (n <= 0 || static_cast<size_t>(n) != bytesToRead) return outcome;
  outcome.version = data[0];

  if (outcome.fileSize > static_cast<size_t>(GLOBAL_STATS_FILE_SIZE) || outcome.version > GLOBAL_STATS_VERSION) {
    outcome.result = StatsLoadResult::NewerFormat;
    return outcome;
  }

  if (n == GLOBAL_STATS_FILE_SIZE_V1 && data[0] == GLOBAL_STATS_VERSION_V1) {
    loadCommonFields(data, out);
    out.completedBooks = 0;
    outcome.result = StatsLoadResult::Ok;
    return outcome;
  }

  if (n == GLOBAL_STATS_FILE_SIZE_V2 && data[0] == GLOBAL_STATS_VERSION_V2) {
    loadCommonFields(data, out);
    out.completedBooks = readLe32(data, 13);
    outcome.result = StatsLoadResult::Ok;
    return outcome;
  }

  if (n != GLOBAL_STATS_FILE_SIZE || data[0] != GLOBAL_STATS_VERSION) return outcome;
  loadCommonFields(data, out);
  out.completedBooks = readLe32(data, 13);
  for (size_t i = 0; i < out.timeOfDaySeconds.size(); ++i) {
    out.timeOfDaySeconds[i] = readLe32(data, 17 + static_cast<int>(i) * 4);
  }
  for (size_t i = 0; i < out.dayOfWeekSeconds.size(); ++i) {
    out.dayOfWeekSeconds[i] = readLe32(data, 33 + static_cast<int>(i) * 4);
  }
  out.readingHistoryAnchorDay = readLe32(data, 61);
  memcpy(out.readingHistoryBits.data(), data + 65, out.readingHistoryBits.size());
  out.longestReadingStreak = readLe16(data, 157);
  outcome.result = StatsLoadResult::Ok;
  return outcome;
}

std::string localSyncedStatsFileName() {
  uint8_t mac[6] = {};
  if (esp_efuse_mac_get_default(mac) != 0) return {};

  char name[32];
  snprintf(name, sizeof(name), "device_%02x%02x%02x%02x%02x%02x.bin", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return name;
}

bool verifyFileSize(const char* path, const size_t expectedSize) {
  FsFile file;
  if (!Storage.openFileForRead("GSTATS", path, file)) return false;
  const size_t actualSize = file.fileSize();
  file.close();
  return actualSize == expectedSize;
}

bool saveToFile(const GlobalReadingStats& stats, const char* path, const char* backupPath) {
  const std::string tmpPath = std::string(path) + ".tmp";
  if (Storage.exists(tmpPath.c_str()) && !Storage.remove(tmpPath.c_str())) {
    LOG_ERR("GSTATS", "Could not remove stale stats temp file: %s", tmpPath.c_str());
    return false;
  }

  FsFile f;
  if (!Storage.openFileForWrite("GSTATS", tmpPath.c_str(), f)) {
    LOG_ERR("GSTATS", "Could not write stats temp file: %s", tmpPath.c_str());
    return false;
  }

  uint8_t data[GLOBAL_STATS_FILE_SIZE];
  serializeStats(stats, data);
  const size_t bytesWritten = f.write(data, GLOBAL_STATS_FILE_SIZE);
  if (bytesWritten != GLOBAL_STATS_FILE_SIZE) {
    LOG_ERR("GSTATS", "Short write for stats temp file %s: %u/%u bytes", tmpPath.c_str(),
            static_cast<unsigned>(bytesWritten), static_cast<unsigned>(GLOBAL_STATS_FILE_SIZE));
    f.close();
    Storage.remove(tmpPath.c_str());
    return false;
  }

  f.flush();
  if (!f.sync()) {
    LOG_ERR("GSTATS", "Failed to sync stats temp file: %s", tmpPath.c_str());
    f.close();
    Storage.remove(tmpPath.c_str());
    return false;
  }

  if (!f.close()) {
    LOG_ERR("GSTATS", "Failed to close stats temp file after save: %s", tmpPath.c_str());
    Storage.remove(tmpPath.c_str());
    return false;
  }

  if (!verifyFileSize(tmpPath.c_str(), GLOBAL_STATS_FILE_SIZE)) {
    LOG_ERR("GSTATS", "Stats temp file has unexpected size: %s", tmpPath.c_str());
    Storage.remove(tmpPath.c_str());
    return false;
  }

  if (backupPath != nullptr) {
    if (Storage.exists(backupPath) && !Storage.remove(backupPath)) {
      LOG_ERR("GSTATS", "Could not remove old stats backup: %s", backupPath);
      Storage.remove(tmpPath.c_str());
      return false;
    }
    if (Storage.exists(path) && !Storage.rename(path, backupPath)) {
      LOG_ERR("GSTATS", "Could not rotate stats backup: %s", path);
      Storage.remove(tmpPath.c_str());
      return false;
    }
  } else if (Storage.exists(path) && !Storage.remove(path)) {
    LOG_ERR("GSTATS", "Could not replace stats file: %s", path);
    Storage.remove(tmpPath.c_str());
    return false;
  }

  if (!Storage.rename(tmpPath.c_str(), path)) {
    LOG_ERR("GSTATS", "Could not replace stats file: %s", path);
    if (backupPath != nullptr && Storage.exists(backupPath) && !Storage.exists(path)) {
      Storage.rename(backupPath, path);
    }
    Storage.remove(tmpPath.c_str());
    return false;
  }
  return true;
}
}  // namespace

static StatsLoadOutcome loadFromFile(const char* path, GlobalReadingStats& out) {
  StatsLoadOutcome outcome;
  FsFile f;
  if (!Storage.openFileForRead("GSTATS", path, f)) return outcome;
  outcome = loadFromOpenFile(f, out);
  f.close();
  return outcome;
}

static GlobalReadingStats loadMain();

GlobalReadingStats GlobalReadingStats::load() {
  GlobalReadingStats stats = loadMain();
  loadDailyReading(stats);
  return stats;
}

static GlobalReadingStats loadMain() {
  GlobalReadingStats stats;
  const StatsLoadOutcome primary = loadFromFile(GLOBAL_STATS_PATH, stats);
  if (primary.result == StatsLoadResult::Ok) return stats;
  if (primary.result == StatsLoadResult::NewerFormat) {
    s_blockDestructiveSave = true;
    LOG_ERR("GSTATS", "On-disk stats are from a newer build (v%u, %u bytes); refusing to overwrite", primary.version,
            static_cast<unsigned>(primary.fileSize));
    return stats;
  }

  const StatsLoadOutcome backup = loadFromFile(GLOBAL_STATS_BAK_PATH, stats);
  if (backup.result == StatsLoadResult::Ok) {
    LOG_DBG("GSTATS", "Recovered global stats from backup");
    return stats;
  }
  if (backup.result == StatsLoadResult::NewerFormat) {
    s_blockDestructiveSave = true;
    LOG_ERR("GSTATS", "Backup stats are from a newer build (v%u, %u bytes); refusing to overwrite", backup.version,
            static_cast<unsigned>(backup.fileSize));
    return stats;
  }

  LOG_DBG("GSTATS", "Global stats missing or corrupt, starting fresh");
  return stats;
}

GlobalReadingStats GlobalReadingStats::loadAggregated() { return loadAggregated(load()); }

bool GlobalReadingStats::hasSyncedStats() {
  FsFile dir = Storage.open(SYNCED_STATS_DIR);
  if (!dir) return false;

  const bool exists = dir.isDirectory();
  dir.close();
  return exists;
}

GlobalReadingStats GlobalReadingStats::loadAggregated(const GlobalReadingStats& localStats) {
  GlobalReadingStats stats = localStats;
  FsFile dir = Storage.open(SYNCED_STATS_DIR);
  if (!dir) return stats;

  if (!dir.isDirectory()) {
    dir.close();
    return stats;
  }

  char name[128];
  const std::string localFileName = localSyncedStatsFileName();
  uint16_t loadedCount = 0;
  uint16_t skippedCount = 0;
  for (FsFile file = dir.openNextFile(); file; file = dir.openNextFile()) {
    const bool isDirectory = file.isDirectory();
    const size_t nameLen = file.getName(name, sizeof(name));

    // Older firmware or manual copies may leave this device's own file here.
    // Skip it because local stats are already included from global_stats.bin.
    if (!isDirectory && nameLen > 0 && (localFileName.empty() || strcmp(name, localFileName.c_str()) != 0)) {
      GlobalReadingStats syncedStats;
      const StatsLoadOutcome outcome = loadFromOpenFile(file, syncedStats);
      if (outcome.result == StatsLoadResult::Ok) {
        addStats(stats, syncedStats);
        loadedCount++;
      } else if (outcome.result == StatsLoadResult::NewerFormat) {
        skippedCount++;
        LOG_DBG("GSTATS", "Skipping newer-format synced stats file: %s (v%u, %u bytes)", name, outcome.version,
                static_cast<unsigned>(outcome.fileSize));
      } else {
        skippedCount++;
        LOG_DBG("GSTATS", "Skipping invalid synced stats file: %s", name);
      }
    }

    file.close();
  }
  dir.close();

  if (loadedCount > 0 || skippedCount > 0) {
    LOG_DBG("GSTATS", "Aggregated %u synced stats file(s), skipped %u", static_cast<unsigned>(loadedCount),
            static_cast<unsigned>(skippedCount));
  }
  return stats;
}

void GlobalReadingStats::save() const {
  if (s_blockDestructiveSave) {
    LOG_ERR("GSTATS", "Refusing to overwrite on-disk stats after newer-format file was detected");
    return;
  }
  saveToFile(*this, GLOBAL_STATS_PATH, GLOBAL_STATS_BAK_PATH);
  saveDailyReading(*this);
}

bool GlobalReadingStats::resetLocal() {
  if (Storage.exists(DAILY_READING_PATH)) Storage.remove(DAILY_READING_PATH);
  return saveToFile(GlobalReadingStats{}, GLOBAL_STATS_PATH, nullptr);
}

void GlobalReadingStats::recordReadingSpan(const ReadingStatsDateTime& localStart, const uint32_t seconds) {
  recordReadingSpanIntoBuckets(timeOfDaySeconds, dayOfWeekSeconds, localStart, seconds);
  recordReadingSpanIntoHistory(readingHistoryAnchorDay, readingHistoryBits, localStart, seconds);
  if (localStart.isValid()) {
    // Split at midnight, as the history does.
    ReadingStatsDateTime cursor = localStart;
    for (uint32_t remaining = seconds; remaining > 0;) {
      const uint32_t untilMidnight = 86400u - (cursor.hour * 3600u + cursor.minute * 60u + cursor.second);
      const uint32_t part = std::min(remaining, untilMidnight);
      addDailySeconds(*this, readingStatsDayIndex(cursor.date), part);
      remaining -= part;
      addSecondsToReadingStatsDateTime(cursor, part);
    }
  }

  const uint16_t historyLongest = computeReadingHistoryLongestStreak(readingHistoryAnchorDay, readingHistoryBits);
  if (historyLongest > longestReadingStreak) {
    longestReadingStreak = historyLongest;
  }
}

uint16_t GlobalReadingStats::currentReadingStreak(const ReadingStatsDate* today) const {
  return computeReadingHistoryCurrentStreak(readingHistoryAnchorDay, readingHistoryBits, today);
}

uint32_t GlobalReadingStats::secondsReadOn(const uint32_t dayIndex) const {
  if (dayIndex > dailyAnchorDay || dailyAnchorDay - dayIndex >= dailySeconds.size()) return 0;
  return dailySeconds[dailyAnchorDay - dayIndex];
}

uint16_t GlobalReadingStats::displayLongestReadingStreak() const {
  return std::max(longestReadingStreak,
                  computeReadingHistoryLongestStreak(readingHistoryAnchorDay, readingHistoryBits));
}
