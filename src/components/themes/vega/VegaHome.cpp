#include "VegaHome.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "RecentBooksStore.h"
#include "VegaTheme.h"
#include "activities/apps/AppRegistry.h"
#include "activities/apps/ClockFace.h"
#include "activities/reader/BookReadingStats.h"
#include "activities/reader/GlobalReadingStats.h"
#include "components/LevelBox.h"
#include "components/StatTile.h"
#include "components/TouchRegistry.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace vegahome {

namespace {

constexpr int kGap = 12;      // between widgets
constexpr int kPad = 12;      // inside a widget's card
constexpr int kRadius = 16;   // the card's corners
constexpr int kBar = 10;      // a progress bar's height
constexpr int kSideMargin = 16;  // the app grid's
constexpr int kDotsHeight = 28;

constexpr uint8_t kS = 1 << Small;
constexpr uint8_t kW = 1 << Wide;
constexpr uint8_t kL = 1 << Large;

const Info kInfo[kTypeCount] = {
    {StrId::STR_WIDGET_CURRENT_BOOK, &icon_reader_24, kS | kW | kL, Wide, BookPace | BookTimeLeft | BookFinish, 3,
     {{StrId::STR_WIDGET_PACE, BookPace}, {StrId::STR_TIME_LEFT, BookTimeLeft},
      {StrId::STR_STATS_EST_FINISH_DATE, BookFinish}},
     false},
    {StrId::STR_WIDGET_RECENT_BOOKS, &icon_library_shelf_24, kS | kW | kL, Wide, CoverProgress, 1,
     {{StrId::STR_STATS_PROGRESS_LBL, CoverProgress}},
     false},
    {StrId::STR_WIDGET_CALENDAR, &icon_calendar_24, kS | kW | kL, Small, ReadDays, 1,
     {{StrId::STR_WIDGET_READ_DAYS, ReadDays}},
     true},
    {StrId::STR_WIDGET_DATE, &icon_calendar_date_24, kS | kW, Small, 0, 0, {}, false},
    {StrId::STR_APP_CLOCK, &icon_clock_24, kS | kW | kL, Small, ClockDate, 2,
     {{StrId::STR_WIDGET_SHOW_DATE, ClockDate}, {StrId::STR_CLOCK_FACE_ANALOG, ClockAnalog}},
     false},
    {StrId::STR_STATS_READING_STREAK_LBL, &icon_cc_warm_24, kS | kW | kL, Small, StreakToday, 1,
     {{StrId::STR_WIDGET_TODAY, StreakToday}},
     false},
};

int widthIn(const uint8_t size) { return size == Small ? 1 : 2; }
int heightIn(const uint8_t size) { return size == Large ? 2 : 1; }
uint8_t sizeOf(const Widget& widget) { return hasSize(widget.type, widget.size) ? widget.size : info(widget.type).defaultSize; }

// The covers' widths as last drawn, per widget and book: the snapshot keeps the covers, the pass over it the rest.
int16_t coverWidths[CrossPointSettings::HOME_WIDGET_MAX][3] = {};

// ---- small drawing helpers ------------------------------------------------------------------------------------

void textAt(const GfxRenderer& r, const int font, const int x, const int y, const char* text,
            const EpdFontFamily::Style style = EpdFontFamily::REGULAR, const bool black = true) {
  r.drawText(font, x, y, text, black, style);
}

void textRight(const GfxRenderer& r, const int font, const int right, const int y, const char* text,
               const EpdFontFamily::Style style = EpdFontFamily::REGULAR) {
  r.drawText(font, right - r.getTextWidth(font, text, style), y, text, true, style);
}

void textCentred(const GfxRenderer& r, const int font, const int cx, const int y, const char* text,
                 const EpdFontFamily::Style style = EpdFontFamily::REGULAR, const bool black = true) {
  r.drawText(font, cx - r.getTextWidth(font, text, style) / 2, y, text, black, style);
}

// The largest of the UI fonts the line fits in, centred.
void fittedCentred(const GfxRenderer& r, const int cx, const int y, const int width, const char* text,
                   const EpdFontFamily::Style style = EpdFontFamily::BOLD) {
  int font = UI_12_FONT_ID;
  if (r.getTextWidth(font, text, style) > width) font = UI_10_FONT_ID;
  if (r.getTextWidth(font, text, style) > width) font = SMALL_FONT_ID;
  textCentred(r, font, cx, y, r.truncatedText(font, text, width, style).c_str(), style);
}

// A level bar on the gray card, white with the read part black (the house level box).
void progressBar(const GfxRenderer& r, const int x, const int y, const int width, const int height,
                 const float percent) {
  levelbox::draw(r, x, y, width, height, height / 2, static_cast<int>(percent / 100.0f * width), nullptr, 0,
                 Color::White);
}

void statTile(const GfxRenderer& r, const Rect& tile, const char* value, const char* label) {
  drawStatTile(r, tile.x, tile.y, tile.width, tile.height, value, label, Color::White, 12);
}

// Up to six tiles, three to a row, filling the box.
void statTiles(const GfxRenderer& r, const Rect& box, const char* const* values, const char* const* labels,
               const int count) {
  if (count <= 0) return;
  const int perRow = std::min(3, count);
  const int rows = (count + perRow - 1) / perRow;
  const int tileW = (box.width - (perRow - 1) * 8) / perRow;
  const int tileH = std::min(84, (box.height - (rows - 1) * 8) / rows);
  for (int i = 0; i < count; ++i) {
    statTile(r, Rect{box.x + (i % perRow) * (tileW + 8), box.y + (i / perRow) * (tileH + 8), tileW, tileH}, values[i],
             labels[i]);
  }
}

// Rounds the corners of the picture at x, y by painting what lies outside a quarter circle in the card's gray,
// with the card's dither phase (anchored to the card's corner).
void roundCorners(const GfxRenderer& r, const Rect& card, const int x, const int y, const int width, const int height,
                  const int radius) {
  r.setDitherOrigin(card.x, card.y);
  for (int row = 0; row < radius; ++row) {
    const float dy = static_cast<float>(radius - row) - 0.5f;
    const int inset = radius - static_cast<int>(std::sqrt(static_cast<float>(radius * radius) - dy * dy));
    if (inset <= 0) continue;
    for (const int edgeY : {y + row, y + height - 1 - row}) {
      r.fillRectDither(x, edgeY, inset, 1, Color::LightGray);
      r.fillRectDither(x + width - inset, edgeY, inset, 1, Color::LightGray);
    }
  }
  r.setDitherOrigin(0, 0);
}

// A book's cover in a box, from Home's thumbnail (homeCoverHeight tall, scaled down for smaller boxes): at the top
// left, or (shelf) centred and standing on the box's bottom. Returns its width.
int drawCover(const GfxRenderer& r, const Rect& card, const RecentBook& book, const Rect& box, const bool shelf) {
  const int thumbHeight = VegaMetrics::values.homeCoverHeight;
  int width = std::min(box.width, box.height * 2 / 3);
  int height = width * 3 / 2;
  HalFile file;
  bool drawn = false;
  if (!book.coverBmpPath.empty() &&
      Storage.openFileForRead("HOME", UITheme::getCoverThumbPath(book.coverBmpPath, thumbHeight), file)) {
    Bitmap bitmap(file);
    if (bitmap.parseHeaders() == BmpReaderError::Ok && bitmap.getWidth() > 0 && bitmap.getHeight() > 0) {
      const float scale = std::min({1.0f, static_cast<float>(box.width) / bitmap.getWidth(),
                                    static_cast<float>(box.height) / bitmap.getHeight()});
      width = static_cast<int>(bitmap.getWidth() * scale);
      height = static_cast<int>(bitmap.getHeight() * scale);
      const int x = shelf ? box.x + (box.width - width) / 2 : box.x;
      const int y = shelf ? box.y + box.height - height : box.y;
      r.drawBitmap(bitmap, x, y, width, height);
      roundCorners(r, card, x, y, width, height, std::min(10, width / 8));
      drawn = true;
    }
    file.close();
  }
  if (!drawn) {
    const int x = shelf ? box.x + (box.width - width) / 2 : box.x;
    const int y = shelf ? box.y + box.height - height : box.y;
    r.fillRoundedRect(x, y, width, height, 10, Color::White);
    drawLucideIcon(r, icon_type_book_24, x + (width - 24) / 2, y + (height - 24) / 2);
  }
  return width;
}

bool hasBooks(const Data& data) { return data.books != nullptr && !data.books->empty(); }

float bookProgress(const Data& data, const int index) {
  return data.progress != nullptr && index < data.progressCount ? data.progress[index] : -1.0f;
}

void emptyNote(const GfxRenderer& r, const Rect& card, const char* text) {
  const int lineH = r.getLineHeight(UI_10_FONT_ID);
  const auto lines = r.wrappedText(UI_10_FONT_ID, text, card.width - 2 * kPad, 3);
  int y = card.y + (card.height - static_cast<int>(lines.size()) * lineH) / 2;
  for (const auto& line : lines) {
    textCentred(r, UI_10_FONT_ID, card.x + card.width / 2, y, line.c_str());
    y += lineH;
  }
}

// ---- the widgets ----------------------------------------------------------------------------------------------

// Where the current book's cover goes in its card.
Rect currentCoverBox(const Rect& card, const uint8_t size, const GfxRenderer& r) {
  const int inner = card.height - 2 * kPad;
  if (size == Small) {
    const int height = inner - 2 * r.getLineHeight(UI_10_FONT_ID) - kBar - 14;
    return Rect{card.x + kPad, card.y + kPad, height * 2 / 3, height};
  }
  const int height = std::min(inner, VegaMetrics::values.homeCoverHeight);
  return Rect{card.x + kPad, card.y + kPad, height * 2 / 3 + 1, height};
}

struct BookFigures {
  char pace[32] = "";
  char left[32] = "";
  char finish[40] = "";
  char paceValue[12] = "";
  char leftValue[16] = "";
  char finishValue[24] = "";
};

BookFigures bookFigures(const Data& data, const uint8_t flags) {
  BookFigures f;
  if (data.stats == nullptr) return f;
  const BookReadingStats& stats = *data.stats;
  const float pace = stats.pagesPerMinute();
  if ((flags & BookPace) && pace > 0.0f) {
    snprintf(f.paceValue, sizeof(f.paceValue), "%.1f", pace);
    snprintf(f.pace, sizeof(f.pace), tr(STR_WIDGET_PACE_FORMAT), pace);
  }
  uint32_t seconds = 0;
  if (stats.isCompleted || !stats.timeLeft(data.progressPercent, seconds)) return f;
  if (flags & BookTimeLeft) {
    formatCompactReadingDuration(seconds, f.leftValue, sizeof(f.leftValue));
    snprintf(f.left, sizeof(f.left), tr(STR_WIDGET_LEFT_FORMAT), f.leftValue);
  }
  ReadingStatsDateTime today;
  if ((flags & BookFinish) && getCurrentLocalReadingStatsDateTime(today)) {
    const ReadingStatsDate date = stats.estimatedFinishDate(today, seconds);
    if (date.isValid()) {
      formatReadingStatsShortDate(date, f.finishValue, sizeof(f.finishValue));
      snprintf(f.finish, sizeof(f.finish), tr(STR_WIDGET_FINISH_FORMAT), f.finishValue);
    }
  }
  return f;
}

void currentBookBackground(GfxRenderer& r, const Rect& card, const uint8_t size, const Data& data, const int index) {
  if (!hasBooks(data)) return;
  coverWidths[index][0] =
      static_cast<int16_t>(drawCover(r, card, data.books->front(), currentCoverBox(card, size, r), false));
}

void currentBookContents(GfxRenderer& r, const Rect& card, const Widget& widget, const uint8_t size, const Data& data,
                         const int index) {
  if (!hasBooks(data)) {
    emptyNote(r, card, tr(STR_NO_OPEN_BOOK));
    return;
  }
  TouchRegistry::getInstance().add(card, kTouchCurrentBook, TouchRegistry::Cover);
  const RecentBook& book = data.books->front();
  const BookFigures f = bookFigures(data, widget.flags);
  const Rect cover = currentCoverBox(card, size, r);
  const int coverW = coverWidths[index][0] > 0 ? coverWidths[index][0] : cover.width;
  const int line10 = r.getLineHeight(UI_10_FONT_ID);
  const int line12 = r.getLineHeight(UI_12_FONT_ID);
  const int lineS = r.getLineHeight(SMALL_FONT_ID);
  const float percent = data.progressPercent;
  char percentText[8] = "";
  if (percent >= 0.0f) snprintf(percentText, sizeof(percentText), "%.0f%%", percent);

  if (size == Small) {
    // The figures beside the cover, the title under it and the bar at the foot.
    const int tx = cover.x + coverW + 10;
    const int tw = card.x + card.width - kPad - tx;
    int y = cover.y;
    if (percentText[0]) {
      textAt(r, UI_12_FONT_ID, tx, y, percentText, EpdFontFamily::BOLD);
      y += line12 + 2;
    }
    for (const char* line : {f.left, f.pace, f.finish}) {
      if (!line[0] || y + lineS > cover.y + cover.height) continue;
      for (const auto& part : r.wrappedText(SMALL_FONT_ID, line, tw, 2)) {
        textAt(r, SMALL_FONT_ID, tx, y, part.c_str());
        y += lineS;
      }
      y += 2;
    }
    const int barY = card.y + card.height - kPad - kBar;
    y = cover.y + cover.height + 6;
    for (const auto& line : r.wrappedText(UI_10_FONT_ID, book.title.c_str(), card.width - 2 * kPad,
                                          std::max(1, (barY - 4 - y) / line10), EpdFontFamily::BOLD)) {
      textAt(r, UI_10_FONT_ID, card.x + kPad, y, line.c_str(), EpdFontFamily::BOLD);
      y += line10;
    }
    if (percent >= 0.0f) progressBar(r, card.x + kPad, barY, card.width - 2 * kPad, kBar, percent);
    return;
  }

  const int tx = cover.x + coverW + 16;
  const int tw = card.x + card.width - kPad - tx;
  // From the foot of the cover up: the bar, the percentage with the time left, the pace and the finish date.
  const int barY = cover.y + cover.height - kBar;
  int rowY = barY;
  if (percent >= 0.0f) {
    rowY = barY - line10 - 4;
    progressBar(r, tx, barY, tw, kBar, percent);
    textAt(r, UI_10_FONT_ID, tx, rowY, percentText, EpdFontFamily::BOLD);
    if (size == Wide && f.left[0]) textRight(r, UI_10_FONT_ID, tx + tw, rowY, f.left);
  }
  if (size == Wide && (f.pace[0] || f.finish[0])) {
    rowY -= lineS + 2;
    const int finishW = f.finish[0] ? r.getTextWidth(SMALL_FONT_ID, f.finish) : 0;
    if (f.finish[0]) textRight(r, SMALL_FONT_ID, tx + tw, rowY, f.finish);
    if (f.pace[0] && r.getTextWidth(SMALL_FONT_ID, f.pace) + finishW + 10 <= tw) {
      textAt(r, SMALL_FONT_ID, tx, rowY, f.pace);
    }
  }
  const int authorH = book.author.empty() ? 0 : line10 + 4;
  const int titleLines = std::clamp((rowY - 6 - cover.y - authorH) / line12, 1, size == Large ? 4 : 3);
  int y = cover.y;
  for (const auto& line : r.wrappedText(UI_12_FONT_ID, book.title.c_str(), tw, titleLines, EpdFontFamily::BOLD)) {
    textAt(r, UI_12_FONT_ID, tx, y, line.c_str(), EpdFontFamily::BOLD);
    y += line12;
  }
  if (!book.author.empty()) {
    textAt(r, UI_10_FONT_ID, tx, y + line10 / 3, r.truncatedText(UI_10_FONT_ID, book.author.c_str(), tw).c_str());
  }
  if (size != Large || data.stats == nullptr) return;

  // The large one: the reading figures as tiles under the cover.
  const BookReadingStats& stats = *data.stats;
  char timeRead[24];
  char sessions[12];
  BookReadingStats::formatDuration(stats.totalReadingSeconds, timeRead, sizeof(timeRead));
  snprintf(sessions, sizeof(sessions), "%u", static_cast<unsigned>(stats.sessionCount));
  const char* values[6];
  const char* labels[6];
  int count = 0;
  values[count] = timeRead;
  labels[count++] = tr(STR_STATS_TIME_LBL);
  values[count] = sessions;
  labels[count++] = tr(STR_STATS_SESSIONS_LBL);
  if (f.paceValue[0]) {
    values[count] = f.paceValue;
    labels[count++] = tr(STR_STATS_PAGES_PER_MIN);
  }
  if (f.leftValue[0]) {
    values[count] = f.leftValue;
    labels[count++] = tr(STR_TIME_LEFT);
  }
  if (f.finishValue[0]) {
    values[count] = f.finishValue;
    labels[count++] = tr(STR_STATS_EST_FINISH_DATE);
  }
  const int top = cover.y + cover.height + kGap;
  statTiles(r, Rect{card.x + kPad, top, card.width - 2 * kPad, card.y + card.height - kPad - top}, values, labels,
            count);
}

// Recent books: the covers side by side, a bar under each; the large one adds the titles.
struct Shelf {
  int count = 0;
  int slotWidth = 0;
  Rect covers;  // the first slot's box; the others follow at slotWidth + gap
  int gap = 0;
};

Shelf shelfOf(const GfxRenderer& r, const Rect& card, const uint8_t size, const Widget& widget, const Data& data) {
  Shelf shelf;
  const int slots = size == Small ? 2 : 3;
  shelf.count = hasBooks(data) ? std::min(slots, static_cast<int>(data.books->size())) : 0;
  shelf.gap = size == Small ? 10 : 16;
  shelf.slotWidth = (card.width - 2 * kPad - (slots - 1) * shelf.gap) / slots;
  const int bars = (widget.flags & CoverProgress) ? kBar - 4 + 8 : 0;
  int height = card.height - 2 * kPad - bars;
  if (size == Large) {
    height = std::min(VegaMetrics::values.homeCoverHeight, height - 2 * r.getLineHeight(UI_10_FONT_ID) - 8);
  }
  shelf.covers = Rect{card.x + kPad, card.y + kPad, shelf.slotWidth, height};
  return shelf;
}

void recentBooksBackground(GfxRenderer& r, const Rect& card, const Widget& widget, const uint8_t size,
                           const Data& data, const int index) {
  const Shelf shelf = shelfOf(r, card, size, widget, data);
  for (int i = 0; i < shelf.count; ++i) {
    Rect box = shelf.covers;
    box.x += i * (shelf.slotWidth + shelf.gap);
    coverWidths[index][i] = static_cast<int16_t>(drawCover(r, card, (*data.books)[i], box, true));
  }
}

void recentBooksContents(GfxRenderer& r, const Rect& card, const Widget& widget, const uint8_t size,
                         const Data& data, const int index) {
  const Shelf shelf = shelfOf(r, card, size, widget, data);
  if (shelf.count == 0) {
    emptyNote(r, card, tr(STR_NO_RECENT_BOOKS));
    return;
  }
  const int line10 = r.getLineHeight(UI_10_FONT_ID);
  for (int i = 0; i < shelf.count; ++i) {
    const int slotX = shelf.covers.x + i * (shelf.slotWidth + shelf.gap);
    const int width = coverWidths[index][i] > 0 ? coverWidths[index][i] : shelf.covers.height * 2 / 3;
    const int x = slotX + (shelf.slotWidth - width) / 2;
    int y = shelf.covers.y + shelf.covers.height + 8;
    if (size == Large) {
      for (const auto& line : r.wrappedText(UI_10_FONT_ID, (*data.books)[i].title.c_str(), shelf.slotWidth, 2,
                                            EpdFontFamily::BOLD)) {
        textCentred(r, UI_10_FONT_ID, slotX + shelf.slotWidth / 2, y, line.c_str(), EpdFontFamily::BOLD);
        y += line10;
      }
      y += 4;
    }
    const float percent = bookProgress(data, i);
    if ((widget.flags & CoverProgress) && percent >= 0.0f) progressBar(r, x, y, width, kBar - 4, percent);
    TouchRegistry::getInstance().add(Rect{slotX, card.y, shelf.slotWidth, card.height}, kTouchRecentBook + i,
                                     TouchRegistry::Cover);
  }
}

// The calendar's mark: read at all (the reading history, two years of it), or for at least the widget's minutes
// (the per-day log, the last two months).
struct MarkContext {
  const GlobalReadingStats* global;
  uint32_t seconds;
};

bool readOn(const calmath::Date& day, const void* context) {
  const auto* mark = static_cast<const MarkContext*>(context);
  if (mark->global == nullptr) return false;
  const uint32_t index = readingStatsDayIndex(
      ReadingStatsDate{static_cast<uint16_t>(day.year), day.month, day.day});
  if (mark->seconds == 0) {
    return readingHistoryHasDay(mark->global->readingHistoryAnchorDay, mark->global->readingHistoryBits, index);
  }
  return mark->global->secondsReadOn(index) >= mark->seconds;
}

// Today as the Clock's calendar sleep screen shows it: the weekday, the day in large digits (clockface size
// `digits`), the month (and the year), centred on cx from top. dayPanelHeight is how tall that is.
int dayPanelHeight(const GfxRenderer& r, const int digits) {
  return r.getLineHeight(UI_12_FONT_ID) + 6 + clockface::digitsHeight(digits) + 6 + r.getLineHeight(UI_10_FONT_ID);
}

void dayPanel(const GfxRenderer& r, const int cx, const int top, const int width, const calmath::DateTime& now,
              const int digits, const bool withYear) {
  char day[4];
  char month[40];
  snprintf(day, sizeof(day), "%d", static_cast<int>(now.date.day));
  if (withYear) {
    snprintf(month, sizeof(month), "%s %d", clockface::monthName(now.date.month), static_cast<int>(now.date.year));
  } else {
    snprintf(month, sizeof(month), "%s", clockface::monthName(now.date.month));
  }
  const int line12 = r.getLineHeight(UI_12_FONT_ID);
  fittedCentred(r, cx, top, width, clockface::weekdayName(calmath::weekday(now.date)));
  clockface::drawDigits(r, digits, day, cx, top + line12 + 6);
  fittedCentred(r, cx, top + line12 + 6 + clockface::digitsHeight(digits) + 6, width, month, EpdFontFamily::REGULAR);
}

void calendarContents(GfxRenderer& r, const Rect& card, const Widget& widget, const uint8_t size, const Data& data) {
  calmath::DateTime now;
  if (!clockface::now(now)) {
    emptyNote(r, card, tr(STR_WIDGET_NO_CLOCK));
    return;
  }
  TouchRegistry::getInstance().add(card, kTouchClock, TouchRegistry::Cover);
  clockface::Options options;
  options.weekStart = data.weekStart;
  const calmath::Date shown{now.date.year, now.date.month, 1};
  const int rows = clockface::monthRows(shown, data.weekStart);
  const MarkContext mark{data.global, static_cast<uint32_t>(widget.value) * 60u};
  const clockface::DayMark marked = (widget.flags & ReadDays) ? readOn : nullptr;
  const int line10 = r.getLineHeight(UI_10_FONT_ID);
  const int line12 = r.getLineHeight(UI_12_FONT_ID);
  char title[40];
  snprintf(title, sizeof(title), "%s %d", clockface::monthName(shown.month), static_cast<int>(shown.year));

  // As in the Clock app: the month's name centred over the grid, today in a black box, the days of the months
  // around it faded. The wide one adds today on the left, as the Clock's calendar sleep screen shows it.
  clockface::MonthGrid grid;
  grid.dayFontId = size == Large ? 0 : SMALL_FONT_ID;
  grid.x = card.x + kPad - 4;
  grid.width = card.width - 2 * kPad + 8;
  if (size == Wide) {
    const int panel = card.width * 2 / 7;
    dayPanel(r, card.x + kPad + panel / 2, card.y + (card.height - dayPanelHeight(r, 1)) / 2, panel, now, 1, false);
    grid.x = card.x + kPad + panel + 8;
    grid.width = card.x + card.width - kPad - grid.x;
  }
  const int titleFont = size == Large ? UI_12_FONT_ID : UI_10_FONT_ID;
  const int titleY = card.y + kPad - (size == Large ? 0 : 4);
  textCentred(r, titleFont, grid.x + grid.width / 2, titleY,
              r.truncatedText(titleFont, size == Small ? clockface::monthName(shown.month) : title, grid.width,
                              EpdFontFamily::BOLD)
                  .c_str(),
              EpdFontFamily::BOLD);
  grid.weekdayY = titleY + (size == Large ? line12 + 8 : line10);
  const bool large = size == Large;
  grid.gridY = grid.weekdayY + (large ? 28 : 18);
  int bottom = card.y + card.height - kPad + (large ? 0 : 4);
  int readDays = 0;
  if (large && marked != nullptr) {
    for (int d = 1; d <= now.date.day; ++d) readDays += readOn(calmath::Date{shown.year, shown.month,
                                                                            static_cast<uint8_t>(d)}, &mark) ? 1 : 0;
    bottom -= r.getLineHeight(SMALL_FONT_ID) + 6;
    char line[48];
    snprintf(line, sizeof(line), tr(STR_WIDGET_DAYS_READ_FORMAT), readDays);
    textCentred(r, SMALL_FONT_ID, card.x + card.width / 2, bottom + 4, line);
  }
  grid.rowHeight = (bottom - grid.gridY) / rows;
  clockface::drawMonthGrid(r, options, grid, shown, now.date, marked, &mark);
}

void dateContents(GfxRenderer& r, const Rect& card, const uint8_t size) {
  calmath::DateTime now;
  if (!clockface::now(now)) {
    emptyNote(r, card, tr(STR_WIDGET_NO_CLOCK));
    return;
  }
  TouchRegistry::getInstance().add(card, kTouchClock, TouchRegistry::Cover);
  if (size == Small) {
    dayPanel(r, card.x + card.width / 2, card.y + (card.height - dayPanelHeight(r, 2)) / 2, card.width - 2 * kPad,
             now, 2, true);
    return;
  }
  // Wide: the day large on the left, the weekday, month and week beside it.
  char day[4];
  char month[40];
  char week[24];
  snprintf(day, sizeof(day), "%d", static_cast<int>(now.date.day));
  snprintf(month, sizeof(month), "%s %d", clockface::monthName(now.date.month), static_cast<int>(now.date.year));
  snprintf(week, sizeof(week), tr(STR_CLOCK_WEEK), calmath::isoWeek(now.date));
  const int line10 = r.getLineHeight(UI_10_FONT_ID);
  const int line12 = r.getLineHeight(UI_12_FONT_ID);
  const int lineS = r.getLineHeight(SMALL_FONT_ID);
  const int half = card.width * 2 / 5;
  clockface::drawDigits(r, 2, day, card.x + half / 2 + kPad, card.y + (card.height - clockface::digitsHeight(2)) / 2);
  const int tx = card.x + half + kPad;
  const int tw = card.x + card.width - kPad - tx;
  int y = card.y + (card.height - line12 - line10 - lineS - 12) / 2;
  const char* weekday = clockface::weekdayName(calmath::weekday(now.date));
  textAt(r, UI_12_FONT_ID, tx, y, r.truncatedText(UI_12_FONT_ID, weekday, tw, EpdFontFamily::BOLD).c_str(),
         EpdFontFamily::BOLD);
  y += line12 + 6;
  textAt(r, UI_10_FONT_ID, tx, y, r.truncatedText(UI_10_FONT_ID, month, tw).c_str());
  textAt(r, SMALL_FONT_ID, tx, y + line10 + 6, week);
}

void clockContents(GfxRenderer& r, const Rect& card, const Widget& widget, const uint8_t size) {
  calmath::DateTime now;
  if (!clockface::now(now)) {
    emptyNote(r, card, tr(STR_WIDGET_NO_CLOCK));
    return;
  }
  TouchRegistry::getInstance().add(card, kTouchClock, TouchRegistry::Cover);
  // The clock keeps clear of the card's edge by more than the other widgets: big digits close to it look cramped.
  constexpr int kMargin = 18;
  const bool twelveHour = SETTINGS.clockFormat == 1;
  const int hour = twelveHour ? (now.hour % 12 == 0 ? 12 : now.hour % 12) : now.hour;
  char hours[4];
  char minutes[4];
  char both[8];
  snprintf(hours, sizeof(hours), twelveHour ? "%d" : "%02d", hour);
  snprintf(minutes, sizeof(minutes), "%02d", now.minute);
  snprintf(both, sizeof(both), "%s:%s", hours, minutes);
  char date[96] = "";
  if (widget.flags & ClockDate) {
    snprintf(date, sizeof(date), tr(STR_CLOCK_DATE_FORMAT), clockface::weekdayName(calmath::weekday(now.date)),
             static_cast<int>(now.date.day), clockface::monthName(now.date.month), static_cast<int>(now.date.year));
  }
  const int dateFont = size == Small ? SMALL_FONT_ID : UI_10_FONT_ID;
  const int dateH = date[0] ? r.getLineHeight(dateFont) + 8 : 0;
  const int cx = card.x + card.width / 2;
  const int boxW = card.width - 2 * kMargin;
  const int boxH = card.height - 2 * kMargin - dateH;
  const int boxTop = card.y + kMargin;
  if (twelveHour) {
    textRight(r, SMALL_FONT_ID, card.x + card.width - kMargin, card.y + kMargin - 8, now.hour < 12 ? "AM" : "PM");
  }

  if ((widget.flags & ClockAnalog) && size == Wide) {
    // The face on the left, the date beside it.
    const int radius = (card.height - 2 * kMargin) / 2;
    clockface::drawAnalog(r, card.x + kMargin + radius, card.y + card.height / 2, radius, now);
    if (date[0]) {
      const int tx = card.x + 2 * kMargin + 2 * radius + 8;
      const int tw = card.x + card.width - kMargin - tx;
      const int line12 = r.getLineHeight(UI_12_FONT_ID);
      const auto lines = r.wrappedText(UI_12_FONT_ID, date, tw, 3, EpdFontFamily::BOLD);
      int y = card.y + (card.height - static_cast<int>(lines.size()) * line12) / 2;
      for (const auto& line : lines) {
        textAt(r, UI_12_FONT_ID, tx, y, line.c_str(), EpdFontFamily::BOLD);
        y += line12;
      }
    }
    return;
  }
  if (widget.flags & ClockAnalog) {
    const int radius = std::min(boxW, boxH) / 2;
    clockface::drawAnalog(r, cx, boxTop + boxH / 2, radius, now);
  } else {
    // The largest digits that fit, on one line or as the hours over the minutes.
    int bestSet = 0;
    bool bestStacked = false;
    int bestHeight = 0;
    for (int set = 2; set >= 0; --set) {
      const int h = clockface::digitsHeight(set);
      const int gap = h / 10;
      if (clockface::digitsWidth(set, both) <= boxW && h <= boxH && h > bestHeight) {
        bestSet = set;
        bestStacked = false;
        bestHeight = h;
      }
      const int stackedW = std::max(clockface::digitsWidth(set, hours), clockface::digitsWidth(set, minutes));
      if (stackedW <= boxW && 2 * h + gap <= boxH && 2 * h + gap > bestHeight) {
        bestSet = set;
        bestStacked = true;
        bestHeight = 2 * h + gap;
      }
    }
    const int h = clockface::digitsHeight(bestSet);
    const int top = boxTop + (boxH - bestHeight) / 2;
    if (bestStacked) {
      clockface::drawDigits(r, bestSet, hours, cx, top);
      clockface::drawDigits(r, bestSet, minutes, cx, top + h + h / 10);
    } else {
      clockface::drawDigits(r, bestSet, both, cx, top);
    }
  }
  if (date[0]) {
    const int y = card.y + card.height - kMargin - r.getLineHeight(dateFont);
    textCentred(r, dateFont, cx, y, r.truncatedText(dateFont, date, boxW, EpdFontFamily::BOLD).c_str(),
                EpdFontFamily::BOLD);
  }
}

// The reading streak, today's reading and (wide) the last seven days as bars.
void streakContents(GfxRenderer& r, const Rect& card, const Widget& widget, const uint8_t size, const Data& data) {
  if (data.global == nullptr) return;
  TouchRegistry::getInstance().add(card, kTouchStats, TouchRegistry::Cover);
  const GlobalReadingStats& global = *data.global;
  ReadingStatsDateTime today;
  const bool hasToday = getCurrentLocalReadingStatsDateTime(today);
  const uint32_t todayIndex = hasToday ? readingStatsDayIndex(today.date) : 0;
  const uint16_t streak = global.currentReadingStreak(hasToday ? &today.date : nullptr);
  const int lineS = r.getLineHeight(SMALL_FONT_ID);
  const int line10 = r.getLineHeight(UI_10_FONT_ID);

  const int unit = size == Large ? (card.height - kGap) / 2 : card.height;
  const int leftW = size == Small ? card.width : card.width * 2 / 5;
  const int cx = card.x + leftW / 2;
  drawLucideIcon(r, icon_cc_warm_24, card.x + kPad, card.y + kPad);
  textAt(r, SMALL_FONT_ID, card.x + kPad + 30, card.y + kPad + 3,
         r.truncatedText(SMALL_FONT_ID, tr(STR_STATS_READING_STREAK_LBL), leftW - 2 * kPad - 30, EpdFontFamily::BOLD)
             .c_str(),
         EpdFontFamily::BOLD);
  char number[8];
  snprintf(number, sizeof(number), "%u", static_cast<unsigned>(streak));
  const int digitsH = clockface::digitsHeight(1);
  const bool showToday = (widget.flags & StreakToday) && hasToday;
  const int body = digitsH + 4 + line10 + (showToday ? lineS + 6 : 0);
  int y = card.y + kPad + 28 + (unit - kPad - 28 - kPad - body) / 2;
  clockface::drawDigits(r, 1, number, cx, y);
  y += digitsH + 4;
  textCentred(r, UI_10_FONT_ID, cx, y, streak == 1 ? tr(STR_STATS_DAY) : tr(STR_STATS_DAYS));
  if (showToday) {
    char minutes[16];
    char line[40];
    formatCompactReadingDuration(global.secondsReadOn(todayIndex), minutes, sizeof(minutes));
    snprintf(line, sizeof(line), "%s: %s", tr(STR_WIDGET_TODAY), minutes);
    textCentred(r, SMALL_FONT_ID, cx, y + line10 + 6, line);
  }
  if (size == Small) return;

  // The last seven days, today on the right and black.
  const int chartX = card.x + leftW;
  const int chartW = card.x + card.width - kPad - chartX;
  const int barW = (chartW - 6 * 8) / 7;
  const int chartTop = card.y + kPad + 4;
  const int chartBottom = card.y + unit - kPad - lineS - 4;
  uint32_t seconds[7] = {};
  uint32_t most = 15 * 60;
  for (int d = 0; d < 7; ++d) {
    seconds[d] = hasToday && todayIndex >= static_cast<uint32_t>(6 - d) ? global.secondsReadOn(todayIndex - (6 - d)) : 0;
    most = std::max(most, seconds[d]);
  }
  for (int d = 0; d < 7; ++d) {
    const int x = chartX + d * (barW + 8);
    const int full = chartBottom - chartTop;
    const int height = std::max(barW / 2, static_cast<int>(static_cast<uint64_t>(full) * seconds[d] / most));
    r.fillRoundedRect(x, chartBottom - full, barW, full, std::min(8, barW / 2), Color::White);
    if (seconds[d] > 0) {
      r.fillRoundedRect(x, chartBottom - height, barW, height, std::min(8, barW / 2),
                        d == 6 ? Color::Black : Color::DarkGray);
    }
    if (hasToday) {
      ReadingStatsDate day = today.date;
      addDaysToReadingStatsDate(day, d - 6);
      char initial[8];
      clockface::weekdayInitials(readingStatsDayOfWeekIndex(day), 1, initial, sizeof(initial));
      textCentred(r, SMALL_FONT_ID, x + barW / 2, chartBottom + 4, initial,
                  d == 6 ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);
    }
  }
  if (size != Large) return;

  char total[24];
  char books[12];
  char longest[12];
  char sessions[12];
  BookReadingStats::formatDuration(global.totalReadingSeconds, total, sizeof(total));
  snprintf(books, sizeof(books), "%u", static_cast<unsigned>(global.completedBooks));
  snprintf(longest, sizeof(longest), "%u", static_cast<unsigned>(global.displayLongestReadingStreak()));
  snprintf(sessions, sizeof(sessions), "%u", static_cast<unsigned>(global.totalSessions));
  const char* values[4] = {total, books, longest, sessions};
  const char* labels[4] = {tr(STR_STATS_TIME_LBL), tr(STR_STATS_COMPLETED_LBL), tr(STR_STATS_LONGEST_STREAK_LBL),
                           tr(STR_STATS_SESSIONS_LBL)};
  const int top = card.y + unit + kGap;
  // Two by two: four figures read better than three and one.
  const int tileW = (card.width - 2 * kPad - 8) / 2;
  const int tileH = (card.y + card.height - kPad - top - 8) / 2;
  for (int i = 0; i < 4; ++i) {
    statTile(r, Rect{card.x + kPad + (i % 2) * (tileW + 8), top + (i / 2) * (tileH + 8), tileW, tileH}, values[i],
             labels[i]);
  }
}

template <typename Draw>
void forEachWidget(const Plan& plan, Draw draw) {
  Place places[CrossPointSettings::HOME_WIDGET_MAX];
  pack(SETTINGS.homeWidgets, SETTINGS.homeWidgetCount, places);
  for (int i = 0; i < SETTINGS.homeWidgetCount; ++i) {
    if (places[i].column < 0) continue;
    const Widget& widget = SETTINGS.homeWidgets[i];
    const uint8_t size = sizeOf(widget);
    draw(i, widget, size, plan.widgetRect(places[i], size));
  }
}

}  // namespace

const Info& info(const uint8_t type) { return kInfo[type < kTypeCount ? type : 0]; }

bool hasSize(const uint8_t type, const uint8_t size) {
  return type < kTypeCount && size < kSizeCount && (kInfo[type].sizes & (1 << size)) != 0;
}

int pack(const Widget* widgets, const int count, Place* places) {
  bool used[kRows][kColumns] = {};
  int rowsUsed = 0;
  for (int i = 0; i < count; ++i) {
    places[i] = Place{};
    if (widgets[i].type >= kTypeCount) continue;
    const uint8_t size = sizeOf(widgets[i]);
    const int w = widthIn(size);
    const int h = heightIn(size);
    for (int row = 0; row + h <= kRows && places[i].column < 0; ++row) {
      for (int column = 0; column + w <= kColumns; ++column) {
        bool free = true;
        for (int dy = 0; dy < h; ++dy) {
          for (int dx = 0; dx < w; ++dx) free = free && !used[row + dy][column + dx];
        }
        if (!free) continue;
        for (int dy = 0; dy < h; ++dy) {
          for (int dx = 0; dx < w; ++dx) used[row + dy][column + dx] = true;
        }
        places[i] = Place{static_cast<int8_t>(column), static_cast<int8_t>(row)};
        rowsUsed = std::max(rowsUsed, row + h);
        break;
      }
    }
  }
  return rowsUsed;
}

bool fits(const Widget* widgets, const int count) {
  if (count > CrossPointSettings::HOME_WIDGET_MAX) return false;
  Place places[CrossPointSettings::HOME_WIDGET_MAX];
  pack(widgets, count, places);
  for (int i = 0; i < count; ++i) {
    if (places[i].column < 0) return false;
  }
  return true;
}

bool hasWidget(const uint8_t type) {
  for (int i = 0; i < SETTINGS.homeWidgetCount; ++i) {
    if (SETTINGS.homeWidgets[i].type == type) return true;
  }
  return false;
}

bool showsTime() { return hasWidget(Clock); }

int Plan::pageCount(const int entries) const {
  const int first = firstCount();
  if (entries <= first) return 1;
  const int other = std::max(1, otherCount());
  return 1 + (entries - first + other - 1) / other;
}

int Plan::pageOf(const int entry) const {
  const int first = firstCount();
  return entry < first ? 0 : 1 + (entry - first) / std::max(1, otherCount());
}

int Plan::firstOn(const int page) const { return page <= 0 ? 0 : firstCount() + (page - 1) * otherCount(); }

Rect Plan::widgetRect(const Place& place, const uint8_t size) const {
  const int rows = heightIn(size);
  return Rect{x + place.column * (columnWidth + gap), widgetTop + place.row * (unitHeight + gap),
              widthIn(size) == 1 ? columnWidth : width, rows * unitHeight + (rows - 1) * gap};
}

Rect Plan::cellRect(const int page, const int slot) const {
  const int top = page == 0 ? firstTop : otherTop;
  const int height = page == 0 ? firstCellHeight : otherCellHeight;
  return Rect{gridLeft + (slot % columns) * cellWidth, top + (slot / columns) * height, cellWidth, height};
}

Plan plan(const GfxRenderer& renderer, const int greetingHeight) {
  const auto& metrics = VegaMetrics::values;
  const int screenW = renderer.getScreenWidth();
  Plan p;
  p.x = metrics.contentSidePadding;
  p.width = screenW - 2 * p.x;
  p.gap = kGap;
  p.columnWidth = (p.width - kGap) / 2;
  p.widgetTop = metrics.homeTopPadding + metrics.topPadding + greetingHeight;
  Place places[CrossPointSettings::HOME_WIDGET_MAX];
  p.widgetRows = pack(SETTINGS.homeWidgets, SETTINGS.homeWidgetCount, places);

  const int bottom = renderer.getScreenHeight() - metrics.buttonHintsHeight - kDotsHeight;
  p.dotsY = bottom + (kDotsHeight - 8) / 2;
  // A cell is as tall as the current book's card (its cover and padding); three rows of them shrink on a short screen.
  const int card = metrics.homeCoverHeight + 2 * kPad;
  p.unitHeight =
      p.widgetRows == 0 ? card : std::min(card, (bottom - p.widgetTop - (p.widgetRows - 1) * kGap) / p.widgetRows);
  const int widgetBottom = p.widgetTop + p.widgetRows * p.unitHeight + std::max(0, p.widgetRows - 1) * kGap;

  // The apps: the chosen rows under the widgets, their plates shrinking to make room; the rows that do not fit move
  // to the next page. The later pages keep the cell size and fill their height.
  p.columns = std::clamp<int>(SETTINGS.homeColumns, 2, 6);
  const int rows = std::clamp<int>(SETTINGS.homeRows, 1, 8);
  p.gridLeft = kSideMargin;
  p.cellWidth = (screenW - 2 * kSideMargin) / p.columns;
  p.iconSize = SETTINGS.homeIconSize == 3 ? 24 : (SETTINGS.homeIconSize == 2 ? 48 : (SETTINGS.homeIconSize == 1 ? 42 : 32));
  const int namesH = SETTINGS.homeShowNames ? 6 + 2 * renderer.getLineHeight(UI_10_FONT_ID) : 0;
  const int minCell = p.iconSize + 15 + namesH + 4;
  p.firstTop = p.widgetRows > 0 ? widgetBottom + kGap + metrics.homeMenuTopOffset : p.widgetTop;
  p.otherTop = metrics.homeTopPadding + metrics.topPadding;
  const int firstH = std::max(0, bottom - p.firstTop);
  const int otherH = std::max(0, bottom - p.otherTop);
  p.firstRows = std::min(rows, firstH / minCell);
  p.firstCellHeight = p.firstRows > 0 ? firstH / p.firstRows : 0;
  p.otherRows = p.firstRows > 0 ? std::clamp(otherH / p.firstCellHeight, 1, 8) : std::max(1, std::min(rows, otherH / minCell));
  p.otherCellHeight = otherH / p.otherRows;
  const int cellH = p.firstRows > 0 ? std::min(p.firstCellHeight, p.otherCellHeight) : p.otherCellHeight;
  // Odd, as fillSquircle wants.
  p.plate = std::max(p.iconSize + 15, std::min(2 * p.iconSize + 5, std::min(p.cellWidth - 8, cellH - namesH - 4))) | 1;
  return p;
}

void drawWidgetBackgrounds(GfxRenderer& renderer, const Plan& plan, const Data& data) {
  forEachWidget(plan, [&](const int i, const Widget& widget, const uint8_t size, const Rect& card) {
    renderer.fillRoundedRect(card.x, card.y, card.width, card.height, kRadius, Color::LightGray);
    if (widget.type == CurrentBook) currentBookBackground(renderer, card, size, data, i);
    if (widget.type == RecentBooks) recentBooksBackground(renderer, card, widget, size, data, i);
  });
}

void drawWidgetContents(GfxRenderer& renderer, const Plan& plan, const Data& data) {
  forEachWidget(plan, [&](const int i, const Widget& widget, const uint8_t size, const Rect& card) {
    switch (widget.type) {
      case CurrentBook:
        currentBookContents(renderer, card, widget, size, data, i);
        if (data.selected) renderer.drawRoundedRect(card.x, card.y, card.width, card.height, 3, kRadius, true);
        break;
      case RecentBooks:
        recentBooksContents(renderer, card, widget, size, data, i);
        break;
      case Calendar:
        calendarContents(renderer, card, widget, size, data);
        break;
      case Date:
        dateContents(renderer, card, size);
        break;
      case Clock:
        clockContents(renderer, card, widget, size);
        break;
      case Streak:
        streakContents(renderer, card, widget, size, data);
        break;
      default:
        break;
    }
    // Edit mode: every widget outlined, the one held more strongly.
    if (data.editing) {
      renderer.drawRoundedRect(card.x, card.y, card.width, card.height, i == data.picked ? 5 : 2, kRadius, true);
    }
  });
}

void drawGrid(GfxRenderer& renderer, const Plan& plan, const int page, const int entries, const int highlighted,
              const std::function<const char*(int)>& label, const std::function<UIIcon(int)>& icon, const bool editing) {
  const int count = page == 0 ? plan.firstCount() : plan.otherCount();
  const int first = plan.firstOn(page);
  const int labelH = renderer.getLineHeight(UI_10_FONT_ID);
  const bool showNames = SETTINGS.homeShowNames != 0;
  const int namesH = showNames ? 6 + 2 * labelH : 0;
  const int contentH = plan.plate + namesH;
  freeink::ui::GfxRendererTarget target(renderer);
  for (int i = first; i < entries && i < first + count; ++i) {
    const Rect cell = plan.cellRect(page, i - first);
    if (!editing) TouchRegistry::getInstance().add(cell, i, TouchRegistry::Item);
    const int cx = cell.x + cell.width / 2;
    const int top = cell.y + std::max(0, cell.height - contentH) / 2;
    const bool on = highlighted == i;
    freeink::ui::fillPlate(target,
                           freeink::ui::Rect{static_cast<int16_t>(cx - plan.plate / 2), static_cast<int16_t>(top),
                                             static_cast<int16_t>(plan.plate), static_cast<int16_t>(plan.plate)},
                           static_cast<freeink::ui::PlateShape>(SETTINGS.homeIconShape),
                           on ? freeink::ui::Paint::solid(freeink::ui::Color::Black)
                              : freeink::ui::Paint::dither(freeink::ui::Color::LightGray));
    if (const freeink::Icon* glyph = homeIcon(icon(i), plan.iconSize)) {
      drawLucideIcon(renderer, *glyph, cx - plan.iconSize / 2, top + (plan.plate - plan.iconSize) / 2, !on);
    }
    // A name that does not fit under its icon wraps to a second line.
    if (!showNames) continue;
    int lineY = top + plan.plate + 6;
    const char* name = label(i);
    for (const auto& line : renderer.wrappedText(UI_10_FONT_ID, name ? name : "", cell.width - 4, 2)) {
      textCentred(renderer, UI_10_FONT_ID, cx, lineY, line.c_str());
      lineY += labelH;
    }
  }
  const int pages = plan.pageCount(entries);
  if (pages > 1) drawPageDots(renderer, 0, plan.dotsY, renderer.getScreenWidth(), pages, page);
}

Hit hitTest(const Plan& plan, const int page, const int entries, const int x, const int y) {
  const auto inside = [x, y](const Rect& r) { return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height; };
  Hit hit;
  if (page == 0) {
    forEachWidget(plan, [&](const int i, const Widget&, const uint8_t, const Rect& card) {
      if (inside(card)) hit = Hit{Hit::Widget, i};
    });
    if (hit.kind != Hit::None) return hit;
  }
  const int count = page == 0 ? plan.firstCount() : plan.otherCount();
  const int first = plan.firstOn(page);
  for (int i = first; i < entries && i < first + count; ++i) {
    if (inside(plan.cellRect(page, i - first))) return Hit{Hit::Entry, i};
  }
  return hit;
}

void moveWidget(const int from, const int to) {
  const int count = SETTINGS.homeWidgetCount;
  if (from < 0 || from >= count || to < 0 || to >= count || from == to) return;
  Widget* widgets = SETTINGS.homeWidgets;
  const Widget moved = widgets[from];
  if (from < to) {
    std::copy(widgets + from + 1, widgets + to + 1, widgets + from);
  } else {
    std::copy_backward(widgets + to, widgets + from, widgets + from + 1);
  }
  widgets[to] = moved;
}

void moveEntry(const std::vector<uint8_t>& keys, const int from, int to) {
  const int count = static_cast<int>(keys.size());
  if (from < 0 || from >= count) return;
  to = std::clamp(to, 0, count - 1);
  std::vector<uint8_t> order = keys;
  const uint8_t moved = order[from];
  order.erase(order.begin() + from);
  order.insert(order.begin() + to, moved);
  SETTINGS.homeOrderCount = static_cast<uint8_t>(std::min<int>(count, CrossPointSettings::HOME_ORDER_MAX));
  std::copy_n(order.begin(), SETTINGS.homeOrderCount, SETTINGS.homeOrder);
}


// ---- Home entries -----------------------------------------------------------------------------------------------

const EntryInfo kEntries[kEntryCount] = {
    {KeyFiles, StrId::STR_BROWSE_FILES, UIIcon::Folder},       {KeyLibrary, StrId::STR_LIBRARY, UIIcon::Library},
    {KeyOpds, StrId::STR_OPDS_BROWSER, UIIcon::Opds},          {KeyStats, StrId::STR_READING_STATS, UIIcon::Chart},
    {KeySaved, StrId::STR_HIGHLIGHTS_HUB, UIIcon::BookmarkIcon}, {KeySettings, StrId::STR_SETTINGS_TITLE, UIIcon::Settings},
};

bool entryHidden(const uint8_t key) {
  if (isAppKey(key)) return apps::isHidden(static_cast<apps::Id>(key & 0x7F));
  return key < 16 && (SETTINGS.homeHiddenEntries & (1u << key)) != 0;
}

void setEntryHidden(const uint8_t key, const bool hidden) {
  if (isAppKey(key)) {
    const uint8_t bit = static_cast<uint8_t>(1u << (key & 0x7F));
    const uint8_t mask = apps::hiddenMask();
    apps::saveHiddenMask(hidden ? mask | bit : mask & ~bit);
    return;
  }
  if (key >= 16) return;
  if (hidden) {
    SETTINGS.homeHiddenEntries |= static_cast<uint16_t>(1u << key);
  } else {
    SETTINGS.homeHiddenEntries &= static_cast<uint16_t>(~(1u << key));
  }
  SETTINGS.saveToFile();
}

// ---- edit mode and the introductions -------------------------------------------------------------------------

void drawPageEdges(const GfxRenderer& renderer, const Plan& plan, const int page, const bool previous,
                   const bool next) {
  const int top = page == 0 && plan.firstRows > 0 ? plan.firstTop : plan.otherTop;
  const int height = plan.dotsY - 8 - top;
  constexpr int kBar = 10;
  if (previous) renderer.fillRoundedRect(4, top, kBar, height, kBar / 2, Color::DarkGray);
  if (next) renderer.fillRoundedRect(renderer.getScreenWidth() - 4 - kBar, top, kBar, height, kBar / 2, Color::DarkGray);
}

namespace {

constexpr int kIntroButtonW = 200;
constexpr int kIntroButtonH = 52;
constexpr int kArrowD = 56;

// A black disc with a white arrow pointing `rotation` from up.
void arrowDisc(const GfxRenderer& r, const int cx, const int cy, const freeink::ui::Rotation rotation) {
  r.fillRoundedRect(cx - kArrowD / 2, cy - kArrowD / 2, kArrowD, kArrowD, kArrowD / 2, Color::Black);
  drawLucideIcon(r, icon_arrow_up_32, cx - 16, cy - 16, false, rotation);
}

// A bold title and its explanation, centred, from y; returns the y under it.
int explained(const GfxRenderer& r, const int cx, int y, const int width, const char* title, const char* text) {
  textCentred(r, UI_12_FONT_ID, cx, y, title, EpdFontFamily::BOLD);
  y += r.getLineHeight(UI_12_FONT_ID) + 2;
  for (const auto& line : r.wrappedText(UI_10_FONT_ID, text, width, 3)) {
    textCentred(r, UI_10_FONT_ID, cx, y, line.c_str());
    y += r.getLineHeight(UI_10_FONT_ID);
  }
  return y;
}

// An icon on a gray disc and its text beside it; returns the y under the row.
int point(const GfxRenderer& r, const int x, const int y, const int width, const freeink::Icon& icon,
          const char* text) {
  constexpr int kDisc = 48;
  r.fillRoundedRect(x, y, kDisc, kDisc, kDisc / 2, Color::LightGray);
  drawLucideIcon(r, icon, x + (kDisc - 24) / 2, y + (kDisc - 24) / 2);
  const int lineH = r.getLineHeight(UI_10_FONT_ID);
  const auto lines = r.wrappedText(UI_10_FONT_ID, text, width - kDisc - 16, 4);
  int ty = y + std::max(0, (kDisc - static_cast<int>(lines.size()) * lineH) / 2);
  for (const auto& line : lines) {
    textAt(r, UI_10_FONT_ID, x + kDisc + 16, ty, line.c_str());
    ty += lineH;
  }
  return std::max(y + kDisc, ty) + 22;
}

}  // namespace

Rect introButton(const GfxRenderer& renderer) {
  const int w = renderer.getScreenWidth();
  const int h = renderer.getScreenHeight();
  return Rect{(w - kIntroButtonW) / 2, h * 2 / 3, kIntroButtonW, kIntroButtonH};
}

void drawIntro(GfxRenderer& renderer, const Intro intro) {
  using freeink::ui::Rotation;
  const int w = renderer.getScreenWidth();
  const int h = renderer.getScreenHeight();
  const int cx = w / 2;
  const int textW = w - 2 * 48;
  renderer.clearScreen();
  if (intro == IntroHome) {
    // Where each gesture starts: the top edge, the sides, the bottom edge.
    arrowDisc(renderer, cx, 24 + kArrowD / 2, Rotation::R180);
    explained(renderer, cx, 24 + kArrowD + 12, textW, tr(STR_INTRO_QUICK_TITLE), tr(STR_INTRO_QUICK_TEXT));
    const int midY = h * 2 / 5;
    arrowDisc(renderer, 16 + kArrowD / 2, midY, Rotation::CCW90);
    arrowDisc(renderer, w - 16 - kArrowD / 2, midY, Rotation::CW90);
    const int pagesW = w - 2 * (kArrowD + 32);
    const int pagesLines =
        static_cast<int>(renderer.wrappedText(UI_10_FONT_ID, tr(STR_INTRO_PAGES_TEXT), pagesW, 3).size());
    const int pagesH = renderer.getLineHeight(UI_12_FONT_ID) + 2 + pagesLines * renderer.getLineHeight(UI_10_FONT_ID);
    explained(renderer, cx, midY - pagesH / 2, pagesW, tr(STR_INTRO_PAGES_TITLE), tr(STR_INTRO_PAGES_TEXT));
    const int bottomArrowY = h - 24 - kArrowD / 2;
    const int lines = static_cast<int>(renderer.wrappedText(UI_10_FONT_ID, tr(STR_INTRO_CUSTOMIZE_TEXT), textW, 3).size());
    const int blockH = renderer.getLineHeight(UI_12_FONT_ID) + 2 + lines * renderer.getLineHeight(UI_10_FONT_ID);
    explained(renderer, cx, bottomArrowY - kArrowD / 2 - 12 - blockH, textW, tr(STR_INTRO_CUSTOMIZE_TITLE),
              tr(STR_INTRO_CUSTOMIZE_TEXT));
    arrowDisc(renderer, cx, bottomArrowY, Rotation::None);
  } else {
    int y = 48;
    textCentred(renderer, UI_12_FONT_ID, cx, y, tr(STR_INTRO_EDIT_TITLE), EpdFontFamily::BOLD);
    y += renderer.getLineHeight(UI_12_FONT_ID) + 28;
    const int x = 32;
    const int width = w - 2 * x;
    y = point(renderer, x, y, width, icon_transfer_arrows_24, tr(STR_INTRO_EDIT_DRAG));
    y = point(renderer, x, y, width, icon_chevrons_right_24, tr(STR_INTRO_EDIT_PAGES));
    y = point(renderer, x, y, width, icon_notes_24, tr(STR_INTRO_EDIT_TAP));
    point(renderer, x, y, width, icon_cc_plus_24, tr(STR_INTRO_EDIT_ADD));
  }
  const Rect button = introButton(renderer);
  renderer.fillRoundedRect(button.x, button.y, button.width, button.height, button.height / 2, Color::Black);
  textCentred(renderer, UI_10_FONT_ID, cx, button.y + (button.height - renderer.getLineHeight(UI_10_FONT_ID)) / 2,
              tr(STR_INTRO_GOT_IT), EpdFontFamily::BOLD, false);
}

}  // namespace vegahome
