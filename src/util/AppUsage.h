#pragma once

#include <string>

#include "util/BatteryHistory.h"

#if NEUINK_BATTERY_HISTORY
#include "activities/apps/AppRegistry.h"

// How long each app (the games and the Clock) was in front today, for the Battery page. Nothing polls: the activity
// manager calls enter() when a screen starts and exit() when it ends, and the time between is added up. Putting
// the reader to sleep ends the app's screen first, so sleeping never counts; waking into the app starts it again.
// The totals sit in RTC memory (24 bytes, like the battery graph) and start again each day.
namespace AppUsage {

void enter(const std::string& activityName);
void exit(const std::string& activityName);

// Seconds in `id` today, including a visit that is still going on.
uint32_t secondsToday(apps::Id id);

}  // namespace AppUsage
#endif
