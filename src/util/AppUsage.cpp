#include "AppUsage.h"

#if NEUINK_BATTERY_HISTORY

#include <Arduino.h>

#include <cstring>

#include "activities/reader/ReadingStatsUtils.h"

#ifndef SIMULATOR
#include <esp_attr.h>
#else
#define RTC_DATA_ATTR
#endif

namespace {
constexpr uint32_t kMagic = 0x41505532;  // "APU2"

struct Totals {
  uint32_t magic;
  uint32_t day;  // the newest local day in the ring
  uint32_t seconds[AppUsage::kDays][apps::kIdCount];  // by day % kDays
};
RTC_DATA_ATTR Totals totals;

// The visit in progress (not kept over a reset: a screen never outlives one).
uint8_t runningId = 0;
unsigned long runningSince = 0;

void clearDay(const uint32_t day) {
  for (uint32_t& value : totals.seconds[day % AppUsage::kDays]) value = 0;
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
  totals.seconds[rollDay() % kDays][id] += (millis() - runningSince) / 1000;
  runningId = 0;
}

uint32_t secondsToday(const apps::Id id) {
  uint32_t seconds = totals.seconds[rollDay() % kDays][id];
  if (runningId == id) seconds += (millis() - runningSince) / 1000;
  return seconds;
}

uint32_t secondsWeek(const apps::Id id) {
  rollDay();
  uint32_t seconds = runningId == id ? (millis() - runningSince) / 1000 : 0;
  for (const auto& day : totals.seconds) seconds += day[id];
  return seconds;
}

}  // namespace AppUsage
#endif
