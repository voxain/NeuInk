#include "AppUsage.h"

#if NEUINK_BATTERY_HISTORY

#include <Arduino.h>

#include "activities/reader/ReadingStatsUtils.h"

#ifndef SIMULATOR
#include <esp_attr.h>
#else
#define RTC_DATA_ATTR
#endif

namespace {
constexpr uint32_t kMagic = 0x41505531;  // "APU1"

struct Totals {
  uint32_t magic;
  uint32_t day;  // the local day the seconds belong to
  uint32_t seconds[apps::kIdCount];
};
RTC_DATA_ATTR Totals totals;

// The visit in progress (not kept over a reset: a screen never outlives one).
uint8_t runningId = 0;
unsigned long runningSince = 0;

// Starts a new day's totals when the local day changed (or the memory is fresh). Without a clock they just go on.
void rollDay() {
  ReadingStatsDateTime now;
  const bool haveClock = getCurrentLocalReadingStatsDateTime(now) && now.isValid();
  const uint32_t day = haveClock ? readingStatsDayIndex(now.date) : 0;
  if (totals.magic != kMagic || (haveClock && totals.day != day)) {
    totals.magic = kMagic;
    totals.day = day;
    for (uint32_t& value : totals.seconds) value = 0;
  }
}

uint8_t appIdOf(const std::string& activityName) {
  const apps::Info* app = apps::findByActivityName(activityName);
  return app ? static_cast<uint8_t>(app->id) : 0;
}
}  // namespace

namespace AppUsage {

void enter(const std::string& activityName) {
  const uint8_t id = appIdOf(activityName);
  if (id == 0 || runningId == id) return;
  runningId = id;
  runningSince = millis();
}

void exit(const std::string& activityName) {
  const uint8_t id = appIdOf(activityName);
  if (id == 0 || runningId != id) return;
  rollDay();
  totals.seconds[id] += (millis() - runningSince) / 1000;
  runningId = 0;
}

uint32_t secondsToday(const apps::Id id) {
  rollDay();
  uint32_t seconds = totals.seconds[id];
  if (runningId == id) seconds += (millis() - runningSince) / 1000;
  return seconds;
}

}  // namespace AppUsage
#endif
