#include "AppUsage.h"

#if NEUINK_BATTERY_HISTORY

#include <Arduino.h>
#include <HalGPIO.h>

#include <cstring>

#include "activities/reader/ReadingStatsUtils.h"

#ifndef SIMULATOR
#include <esp_attr.h>
#else
#define RTC_NOINIT_ATTR
#endif

namespace {
constexpr uint32_t kMagic = 0x41505534;  // "APU4"

struct Totals {
  uint32_t magic;
  uint32_t day;  // the newest local day in the ring
  uint32_t seconds[AppUsage::kDays][AppUsage::kSlots];   // by day % kDays
  uint32_t measured[AppUsage::kDays][AppUsage::kSlots];  // seconds a charge loss was measured over
  uint16_t drain[AppUsage::kDays][AppUsage::kSlots];     // percent lost in them
};
// RTC_NOINIT, not RTC_DATA: see BatteryHistory.cpp. The magic tells a valid copy from power-on garbage.
RTC_NOINIT_ATTR Totals totals;

// The visit in progress (not kept over a reset: a screen never outlives one).
uint8_t runningId = 0;
unsigned long runningSince = 0;

// The gauge read the next one is measured from; dropped whenever the screen changes (not kept over a reset).
bool baselineValid = false;
uint8_t baselinePercent = 0;
unsigned long baselineMs = 0;
constexpr unsigned long kLongestGapMs = 2UL * 60 * 60 * 1000;
constexpr uint32_t kMinMeasuredSeconds = 15 * 60;

void clearDay(const uint32_t day) {
  const uint32_t d = day % AppUsage::kDays;
  for (uint32_t& value : totals.seconds[d]) value = 0;
  for (uint32_t& value : totals.measured[d]) value = 0;
  for (uint16_t& value : totals.drain[d]) value = 0;
}

// Brings the ring up to today: the days since the last visit start empty, and memory that is fresh, a clock that
// went back or a week without any visit starts over. Without a clock it all stays on the one day.
uint32_t rollDay() {
  ReadingStatsDateTime now;
  const bool haveClock = getCurrentLocalReadingStatsDateTime(now) && now.isValid();
  if (!haveClock && totals.magic == kMagic) return totals.day;
  const uint32_t day = haveClock ? readingStatsDayIndex(now.date) : 0;
  if (totals.magic != kMagic || day < totals.day || day - totals.day >= AppUsage::kDays) {
    memset(&totals, 0, sizeof(totals));
    totals.magic = kMagic;
  } else {
    for (uint32_t d = totals.day + 1; d <= day; ++d) clearDay(d);
  }
  totals.day = day;
  return day;
}

// The slot a screen belongs to, or 0 for one that is only part of another's visit.
uint8_t slotOf(const std::string& activityName) {
  if (activityName == "Reader" || activityName == "EpubReader" || activityName == "TxtReader" ||
      activityName == "XtcReader") {
    return AppUsage::kReader;
  }
  const apps::Info* app = apps::findByActivityName(activityName);
  return app ? static_cast<uint8_t>(app->id) : 0;
}
}  // namespace

namespace AppUsage {

void enter(const std::string& activityName) {
  const uint8_t id = slotOf(activityName);
  if (id == 0 || runningId == id) return;
  runningId = id;
  runningSince = millis();
  baselineValid = false;
}

void exit(const std::string& activityName) {
  const uint8_t id = slotOf(activityName);
  if (id == 0 || runningId != id) return;
  totals.seconds[rollDay() % kDays][id] += (millis() - runningSince) / 1000;
  runningId = 0;
  baselineValid = false;
}

size_t stateSize() { return sizeof(totals); }

bool saveState(uint8_t* out) {
  if (totals.magic != kMagic) return false;
  memcpy(out, &totals, sizeof(totals));
  return true;
}

bool restoreState(const uint8_t* in) {
  if (totals.magic == kMagic) return false;
  Totals restored;
  memcpy(&restored, in, sizeof(restored));
  if (restored.magic != kMagic) return false;
  totals = restored;  // rollDay() later empties the days that have passed
  return true;
}

void chargeSample(const uint8_t percent) {
  const unsigned long now = millis();
  if (gpio.isUsbConnected()) {
    baselineValid = false;
    return;
  }
  if (baselineValid && now - baselineMs < kLongestGapMs && percent <= baselinePercent) {
    const uint32_t day = rollDay() % kDays;
    totals.drain[day][runningId] += baselinePercent - percent;
    totals.measured[day][runningId] += (now - baselineMs) / 1000;
  }
  baselinePercent = percent;
  baselineMs = now;
  baselineValid = true;
}

int drainTenthsPerHour(const uint8_t slot) {
  rollDay();
  uint32_t seconds = 0;
  uint32_t percent = 0;
  for (int d = 0; d < kDays; ++d) {
    seconds += totals.measured[d][slot];
    percent += totals.drain[d][slot];
  }
  if (seconds < kMinMeasuredSeconds) return -1;
  return static_cast<int>(percent * 36000UL / seconds);
}

uint32_t secondsToday(const uint8_t id) {
  uint32_t seconds = totals.seconds[rollDay() % kDays][id];
  if (runningId == id) seconds += (millis() - runningSince) / 1000;
  return seconds;
}

uint32_t secondsWeek(const uint8_t id) {
  rollDay();
  uint32_t seconds = runningId == id ? (millis() - runningSince) / 1000 : 0;
  for (const auto& day : totals.seconds) seconds += day[id];
  return seconds;
}

}  // namespace AppUsage
#endif
