#include "UsageSnapshot.h"

#if NEUINK_BATTERY_HISTORY

#include <HalStorage.h>
#include <Logging.h>

#include <cstring>
#include <vector>

#include "util/AppUsage.h"

namespace {
constexpr char kPath[] = "/.crosspoint/usage.bin";
constexpr char kTag[] = "USG";
constexpr uint32_t kMagic = 0x5553474E;  // "USGN"

struct Header {
  uint32_t magic;
  uint16_t historySize;
  uint16_t usageSize;
};
}  // namespace

namespace UsageSnapshot {

void save() {
  const size_t usageSize = AppUsage::stateSize();
  std::vector<uint8_t> data(sizeof(Header) + BatteryHistory::kStateSize + usageSize);
  const Header header{kMagic, static_cast<uint16_t>(BatteryHistory::kStateSize), static_cast<uint16_t>(usageSize)};
  memcpy(data.data(), &header, sizeof(header));
  uint8_t history[BatteryHistory::kStateSize];
  // Never overwrite a good file with an empty or garbage copy.
  if (!BatteryHistory::saveState(history) ||
      !AppUsage::saveState(data.data() + sizeof(header) + sizeof(history))) {
    return;
  }
  memcpy(data.data() + sizeof(header), history, sizeof(history));

  HalFile file;
  if (!Storage.openFileForWrite(kTag, kPath, file)) {
    LOG_ERR(kTag, "Cannot write %s", kPath);
    return;
  }
  const size_t written = file.write(data.data(), data.size());
  file.close();
  if (written != data.size()) LOG_ERR(kTag, "Short write to %s", kPath);
}

void restore() {
  HalFile file;
  if (!Storage.exists(kPath) || !Storage.openFileForRead(kTag, kPath, file)) return;
  Header header{};
  std::vector<uint8_t> data(AppUsage::stateSize());
  uint8_t history[BatteryHistory::kStateSize];
  const bool ok = file.read(&header, sizeof(header)) == static_cast<int>(sizeof(header)) && header.magic == kMagic &&
                  header.historySize == sizeof(history) && header.usageSize == data.size() &&
                  file.read(history, sizeof(history)) == static_cast<int>(sizeof(history)) &&
                  file.read(data.data(), data.size()) == static_cast<int>(data.size());
  file.close();
  if (!ok) return;  // another firmware's layout: start over
  BatteryHistory::restoreState(history);
  AppUsage::restoreState(data.data());
}

}  // namespace UsageSnapshot
#endif
