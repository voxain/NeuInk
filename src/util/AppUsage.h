#pragma once

#include <cstddef>
#include <string>

#include "util/BatteryHistory.h"

#if NEUINK_BATTERY_HISTORY
#include "activities/apps/AppRegistry.h"

// How long each app (everything with an icon on Home, Home itself and the book reader) was in front, today and over
// the last week, for
// the Battery page. Nothing polls: the activity manager calls enter() when a screen starts and exit() when it ends,
// and the time between is added up. The reader's own screens (chapters, the menu, a dictionary) are sub-screens
// of its visit, which lasts as long as a book screen is open. Putting the reader to sleep ends the screen first,
// so sleeping never counts; waking into the app starts it again.
// It also learns what each costs in charge. Every gauge read the battery graph takes (chargeSample) adds the percent
// lost since the previous read, and the seconds between the two, to whatever was in front; an interval that spans a
// change of screen, a charge or a long gap is dropped, so sleep and charging never count. The percent lost per hour
// of each is then the week's loss over the week's measured time. A screen of no slot of its own (a Settings page, a
// panel over Home) counts toward the one it opened over; slot 0 is what runs outside all of them (sleep, boot).
// The totals sit in RTC memory (a few numbers per slot and day: about 0.5 KB, no timestamps, like the battery graph).
namespace AppUsage {

void enter(const std::string& activityName);
void exit(const std::string& activityName);

constexpr int kDays = 7;

// What is timed: a game's or the Clock's id, then the reader, Home and the other Home entries.
constexpr uint8_t kReader = apps::kIdCount;
constexpr uint8_t kHome = kReader + 1;
constexpr uint8_t kSettings = kReader + 2;
constexpr uint8_t kFiles = kReader + 3;
constexpr uint8_t kLibrary = kReader + 4;
constexpr uint8_t kOpds = kReader + 5;
constexpr uint8_t kStats = kReader + 6;
constexpr uint8_t kSaved = kReader + 7;
constexpr uint8_t kSlots = kReader + 8;

// The totals as bytes, for UsageSnapshot's file (see BatteryHistory::saveState).
size_t stateSize();
bool saveState(uint8_t* out);  // false while RTC memory holds nothing valid
bool restoreState(const uint8_t* in);

// A gauge read of `percent`, now.
void chargeSample(uint8_t percent);

// Percent of charge lost per hour in a slot over the last kDays days, in tenths; -1 until there are 15 minutes of
// measurement to tell it from.
int drainTenthsPerHour(uint8_t slot);

// Seconds in a slot today, or over the last kDays days, including a visit that is still going on.
uint32_t secondsToday(uint8_t slot);
uint32_t secondsWeek(uint8_t slot);

}  // namespace AppUsage
#endif
