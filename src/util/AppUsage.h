#pragma once

#include <string>

#include "util/BatteryHistory.h"

#if NEUINK_BATTERY_HISTORY
#include "activities/apps/AppRegistry.h"

// How long each app (the games and the Clock) and the book reader was in front, today and over the last week, for
// the Battery page. Nothing polls: the activity manager calls enter() when a screen starts and exit() when it ends,
// and the time between is added up. The reader's own screens (chapters, the menu, a dictionary) are sub-screens
// of its visit, which lasts as long as a book screen is open. Putting the reader to sleep ends the screen first,
// so sleeping never counts; waking into the app starts it again.
// The totals sit in RTC memory (one number per slot and day: 224 bytes, no timestamps, like the battery graph).
namespace AppUsage {

void enter(const std::string& activityName);
void exit(const std::string& activityName);

constexpr int kDays = 7;

// What is timed: an app's id, or the reader, which is not one of the apps.
constexpr uint8_t kReader = apps::kIdCount;
constexpr uint8_t kSlots = kReader + 1;

// Seconds in a slot today, or over the last kDays days, including a visit that is still going on.
uint32_t secondsToday(uint8_t slot);
uint32_t secondsWeek(uint8_t slot);

}  // namespace AppUsage
#endif
