#pragma once

#include <GfxRenderer.h>

#include <cstddef>
#include <cstdint>

#include "util/CalendarMath.h"

// What the Clock app and the Clock & Calendar sleep screen draw: the time or a
// large date above a month. Kept apart from the app's screen so the sleep
// screen can paint the same calendar without it.
namespace clockface {

enum Face : uint8_t { Digital = 0, Analog = 1, PersianDays = 2, kFaceCount = 3 };
enum WeekStart : uint8_t { Monday = 0, Saturday = 1, Sunday = 2, kWeekStartCount = 3 };

// The Clock settings (Settings > Apps > Clock), kept on the card.
struct Options {
  uint8_t face = Digital;
  bool persianDate = false;
  uint8_t weekStart = Monday;
  bool weekNumber = true;

  // The face to draw: the one with Persian days needs the Persian date on.
  Face shownFace() const { return (face == PersianDays && !persianDate) || face >= kFaceCount ? Digital : static_cast<Face>(face); }
};

void loadOptions(Options& options);
void saveOptions(const Options& options);

// The local date and time; false while the clock has never been set.
bool now(calmath::DateTime& out);
const char* monthName(int month);      // 1..12
const char* weekdayName(int weekday);  // 0 = Monday
// The first `count` letters of a weekday's name.
void weekdayInitials(int weekday, int count, char* out, size_t size);

struct Layout {
  int dateY = 0;
  int monthY = 0;
  int weekdayY = 0;
  int gridY = 0;
  int rowHeight = 0;
  int rows = 5;
};

// Where the parts of the app's screen sit below `top` for the month `shown` (its first day).
Layout appLayout(const Options& options, int top, const calmath::Date& shown);

// The app: clock, date lines, month title with arrows and the month `shown`.
void drawApp(const GfxRenderer& renderer, const Options& options, int top, const calmath::DateTime& time,
             const calmath::Date& shown, bool showsCurrentMonth);
// A month as a grid: the weekday initials at weekdayY, the days from gridY in rows of rowHeight, across x .. x + width.
struct MonthGrid {
  int x = 0;
  int width = 0;
  int weekdayY = 0;
  int gridY = 0;
  int rowHeight = 0;
  int dayFontId = 0;  // 0: the UI's 12 pt, anything else draws a compact grid (one-letter weekdays)
  bool secondNumbers = false;  // the Persian day under each day
};
// Whether a day gets a dot under its number (Home's calendar: the days read on).
using DayMark = bool (*)(const calmath::Date& day, const void* context);
void drawMonthGrid(const GfxRenderer& renderer, const Options& options, const MonthGrid& grid, const calmath::Date& shown,
                   const calmath::Date& today, DayMark marked = nullptr, const void* context = nullptr);
int monthRows(const calmath::Date& shown, uint8_t weekStart);  // 4 to 6

// The analog face centred on cx, cy (the app's has a radius of 128).
void drawAnalog(const GfxRenderer& renderer, int cx, int cy, int radius, const calmath::DateTime& time);

// The clock digits ('0'..'9' and ':') in three sizes: 0 = 38, 1 = 74, 2 = 105 px tall, tops at y.
int digitsHeight(int size);
int digitsWidth(int size, const char* text);
void drawDigits(const GfxRenderer& renderer, int size, const char* text, int centreX, int y);

// The sleep screen: weekday, a large day number, the month and its calendar.
// No time, so it stays right until midnight.
void drawSleepCalendar(const GfxRenderer& renderer, const Options& options, const calmath::DateTime& time);

}  // namespace clockface
