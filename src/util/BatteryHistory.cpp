#include "BatteryHistory.h"

#if NEUINK_BATTERY_HISTORY

#include <HalPowerManager.h>

#include <cstring>

#include "activities/reader/ReadingStatsUtils.h"

#ifndef SIMULATOR
#include <esp_attr.h>
#else
#define RTC_DATA_ATTR
#endif

namespace {
constexpr uint32_t kMagic = 0x42415431;  // "BAT1"
constexpr uint32_t kBucketMinutes = 30;

struct History {
  uint32_t magic;
  uint32_t newest;  // the half hour (minutes since day 0 / 30) of the latest sample
  uint8_t levels[BatteryHistory::kBuckets];
};
RTC_DATA_ATTR History history;

// The current half hour, from the local clock; false while there is none.
bool currentBucket(uint32_t& bucket) {
  ReadingStatsDateTime now;
  if (!getCurrentLocalReadingStatsDateTime(now) || !now.isValid()) return false;
  const uint64_t minutes = static_cast<uint64_t>(readingStatsDayIndex(now.date)) * 1440 + now.hour * 60 + now.minute;
  bucket = static_cast<uint32_t>(minutes / kBucketMinutes);
  return true;
}

void clear(const uint32_t bucket) {
  history.magic = kMagic;
  history.newest = bucket;
  memset(history.levels, BatteryHistory::kNone, sizeof(history.levels));
}

// Brings the ring up to `bucket`: half hours since the last sample are empty; a clock that went back, or a gap
// of a day, starts again.
void advance(const uint32_t bucket) {
  if (history.magic != kMagic || bucket < history.newest || bucket - history.newest >= BatteryHistory::kBuckets) {
    clear(bucket);
    return;
  }
  for (uint32_t b = history.newest + 1; b <= bucket; ++b) history.levels[b % BatteryHistory::kBuckets] = BatteryHistory::kNone;
  history.newest = bucket;
}
}  // namespace

namespace BatteryHistory {

void sample() {
  uint32_t bucket;
  if (!currentBucket(bucket)) return;
  advance(bucket);
  const uint16_t percent = powerManager.getBatteryPercentage();
  history.levels[bucket % kBuckets] = static_cast<uint8_t>(percent > 100 ? 100 : percent);
}

bool read(uint8_t (&levels)[kBuckets]) {
  uint32_t bucket;
  if (!currentBucket(bucket)) return false;
  advance(bucket);
  for (int i = 0; i < kBuckets; ++i) levels[i] = history.levels[(bucket + 1 + i) % kBuckets];
  return true;
}

}  // namespace BatteryHistory
#endif
