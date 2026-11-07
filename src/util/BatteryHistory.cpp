#include "BatteryHistory.h"

#if NEUINK_BATTERY_HISTORY

#include <algorithm>
#include <cstring>

#include "activities/reader/ReadingStatsUtils.h"
#include "util/AppUsage.h"

#ifndef SIMULATOR
#include <esp_attr.h>
#else
#define RTC_NOINIT_ATTR
#endif

namespace {
constexpr uint32_t kMagic = 0x42415433;  // "BAT3"

struct History {
  uint32_t magic;
  uint32_t newest;  // the bucket (minutes since day 0 / kBucketMinutes) of the latest sample
  uint8_t levels[BatteryHistory::kBuckets];
};
// RTC_NOINIT, not RTC_DATA: plain RTC data is set to its initial value again on a software reset (an update, a Wi-Fi
// session's restart); only deep sleep keeps it. The magic tells a valid copy from the garbage of a power-on.
RTC_NOINIT_ATTR History history;

// The current bucket, from the local clock; false while there is none.
bool currentBucket(uint32_t& bucket) {
  ReadingStatsDateTime now;
  if (!getCurrentLocalReadingStatsDateTime(now) || !now.isValid()) return false;
  const uint64_t minutes = static_cast<uint64_t>(readingStatsDayIndex(now.date)) * 1440 + now.hour * 60 + now.minute;
  bucket = static_cast<uint32_t>(minutes / BatteryHistory::kBucketMinutes);
  return true;
}

void clear(const uint32_t bucket) {
  history.magic = kMagic;
  history.newest = bucket;
  memset(history.levels, BatteryHistory::kNone, sizeof(history.levels));
}

// Brings the ring up to `bucket`: buckets since the last sample are empty; a clock that went back, or a gap
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

void note(const uint16_t percent) {
  AppUsage::chargeSample(static_cast<uint8_t>(percent > 100 ? 100 : percent));
  uint32_t bucket;
  if (!currentBucket(bucket)) return;
  advance(bucket);
  history.levels[bucket % kBuckets] = static_cast<uint8_t>(percent > 100 ? 100 : percent);
}

static_assert(sizeof(History) == kStateSize, "History is the file's layout");

bool saveState(uint8_t (&out)[kStateSize]) {
  if (history.magic != kMagic) return false;
  memcpy(out, &history, kStateSize);
  return true;
}

bool restoreState(const uint8_t (&in)[kStateSize]) {
  if (history.magic == kMagic) return false;
  History restored;
  memcpy(&restored, in, kStateSize);
  if (restored.magic != kMagic) return false;
  history = restored;  // advance() later empties the buckets the time away covered
  return true;
}

Rates rates(const uint8_t (&levels)[kBuckets]) {
  constexpr int kSleepGapBuckets = 30 / kBucketMinutes;
  constexpr int kAwakeMinBuckets = 30 / kBucketMinutes;  // half an hour of neighbouring points
  constexpr int kSleepMinBuckets = 60 / kBucketMinutes;  // an hour of gap
  int awakeDrop = 0, awakeBuckets = 0, sleepDrop = 0, sleepBuckets = 0;
  int previous = -1;
  for (int i = 0; i < kBuckets; ++i) {
    if (levels[i] == kNone) continue;
    if (previous >= 0 && levels[i] <= levels[previous]) {  // any rise is a charge, not a drain
      const int drop = levels[i] < levels[previous] ? levels[previous] - levels[i] : 0;
      const int gap = i - previous;
      if (gap == 1) {
        awakeDrop += drop;
        awakeBuckets += 1;
      } else if (gap >= kSleepGapBuckets) {
        sleepDrop += drop;
        sleepBuckets += gap;
      }
    }
    previous = i;
  }
  Rates result;
  const int perHour = 60 / kBucketMinutes;
  if (awakeBuckets >= kAwakeMinBuckets && awakeDrop > 0) result.awakeTenths = awakeDrop * 10 * perHour / awakeBuckets;
  if (sleepBuckets >= kSleepMinBuckets) {
    result.sleepTenths = (sleepDrop > 0 ? sleepDrop : 1) * 10 * perHour / sleepBuckets;
    if (result.sleepTenths == 0) result.sleepTenths = 1;
  }
  const int measured = awakeBuckets + sleepBuckets;
  if (measured >= kAwakeMinBuckets && awakeDrop + sleepDrop > 0) {
    result.overallTenths = std::max(1, (awakeDrop + sleepDrop) * 10 * perHour / measured);
  }
  return result;
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
