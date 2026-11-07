#pragma once

#include <BoardConfig.h>

#include <cstddef>
#include <cstdint>

// The battery level over the last 24 hours, for the Battery page in Settings. Only the Xteink X4 Pro has the
// CW2017 fuel gauge it reads (voltage, charge percentage and temperature; no cycle count or health), so only
// that build carries it.
//
// Made to cost nothing: one byte per five minutes (288 bytes), kept in RTC memory, which survives deep sleep and
// soft resets, so it never touches the SD card or flash. It never reads the gauge itself: it takes the level from
// the reads that happen anyway (every status bar redraw, the charging poll), through HalPowerManager's observer.
// Nothing runs during deep sleep; the five minutes the reader slept through stay empty and show as gaps.
#define NEUINK_BATTERY_HISTORY FREEINK_DEVICE_X4PRO

#if NEUINK_BATTERY_HISTORY
namespace BatteryHistory {

constexpr int kBucketMinutes = 5;
constexpr int kBuckets = 24 * 60 / kBucketMinutes;    // 24 hours of five-minute buckets
constexpr uint8_t kNone = 0xFF;                       // a bucket without a sample

// The graph as bytes, for UsageSnapshot's file: RTC memory does not survive everything (a power-off, an update).
// restoreState() only takes them when RTC memory holds nothing of its own.
constexpr size_t kStateSize = 8 + kBuckets;
bool saveState(uint8_t (&out)[kStateSize]);  // false while RTC memory holds nothing valid
bool restoreState(const uint8_t (&in)[kStateSize]);

// Stores a level the gauge was just read at, in this bucket. Needs the clock; without one it does nothing.
void note(uint16_t percent);

// What the last 24 hours say about the drain, in tenths of a percent lost per hour; -1 where they do not tell yet.
// Awake: the loss between neighbouring points (the status bar reads the gauge on every redraw). Asleep: the loss
// across a gap of at least half an hour, where no point exists because nothing ran (a recharge ends a pair).
// A sleep that lost nothing counts as one percent, so that estimate is a lower bound of the runtime.
struct Rates {
  int awakeTenths = -1;
  int sleepTenths = -1;
};
Rates rates(const uint8_t (&levels)[kBuckets]);

// The last 24 hours, oldest first, the last entry being the current bucket: a level 0-100 or kNone.
// Returns false when there is no clock.
bool read(uint8_t (&levels)[kBuckets]);

}  // namespace BatteryHistory
#endif
