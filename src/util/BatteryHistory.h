#pragma once

#include <BoardConfig.h>

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

// Stores a level the gauge was just read at, in this bucket. Needs the clock; without one it does nothing.
void note(uint16_t percent);

// The last 24 hours, oldest first, the last entry being the current bucket: a level 0-100 or kNone.
// Returns false when there is no clock.
bool read(uint8_t (&levels)[kBuckets]);

}  // namespace BatteryHistory
#endif
