#pragma once

#include <I18n.h>

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "CrossPointSettings.h"
#include "components/themes/BaseTheme.h"

class GfxRenderer;
struct BookReadingStats;
struct GlobalReadingStats;
struct RecentBook;
namespace freeink {
struct Icon;
}

// The Vega Home screen: widgets on a grid two cells wide and three high at the top of the first page, the app grid
// under them and on the pages after it. The widgets are kept in SETTINGS.homeWidgets in their order; each takes the
// first free place it fits in, row by row, so reordering is moving one in that list.
namespace vegahome {

using Widget = CrossPointSettings::HomeWidget;

enum Type : uint8_t {
  CurrentBook = 0,
  RecentBooks,
  Calendar,
  Date,
  Clock,
  Streak,
  ReadingGoal,
  UpNext,
  Highlight,
  Battery,
  kTypeCount
};
enum Size : uint8_t { Small = 0, Wide = 1, Large = 2, kSizeCount };  // 1x1, 2x1, 2x2 cells

// The widgets' optional data, a bit each in Widget::flags (the meaning depends on the type).
enum Flag : uint8_t {
  BookPace = 1,  // CurrentBook
  BookTimeLeft = 2,
  BookFinish = 4,
  BookAuthor = 8,
  CoverProgress = 1,  // RecentBooks
  ReadDays = 1,       // Calendar: a dot on the days read on (Widget::value: the minutes it takes, 0 = any)
  ClockDate = 1,      // Clock (Widget::value: the digits' size, 0 = the largest that fits)
  ClockAnalog = 2,
  StreakToday = 1,  // Streak
  WeekFromSunday = 2,  // Streak and ReadingGoal: their week's bars from Sunday instead of Monday
  BatteryRuntime = 1,  // Battery: how long the charge lasts
};

struct Option {
  StrId label;
  uint8_t flag;
};

// The one number a widget may have (Widget::value), set with - and + in its options.
struct Value {
  enum Format : uint8_t { None, MinutesOrAny, Minutes, DigitSize } format = None;
  StrId label{};
  uint8_t step = 1;
  uint8_t max = 0;
  uint8_t initial = 0;
};

struct Info {
  StrId name;
  const freeink::Icon* icon;  // 24 px
  uint8_t sizes;              // a bit per Size it comes in
  Size defaultSize;
  uint8_t defaultFlags;
  uint8_t optionCount;
  Option options[4];
  Value value;
};

// The value as its options show it ("Any", "15 min", "Auto", "L").
void valueText(const Value& value, uint8_t number, char* out, size_t size);

const Info& info(uint8_t type);
bool hasSize(uint8_t type, uint8_t size);

constexpr int kColumns = 2;
constexpr int kRows = 3;

// Where each widget sits on the 2x3 grid (column, row), -1 when it does not fit. Returns the rows used.
struct Place {
  int8_t column = -1;
  int8_t row = -1;
};
int pack(const Widget* widgets, int count, Place* places);
// Whether all of them fit.
bool fits(const Widget* widgets, int count);
bool hasWidget(uint8_t type);

// What a Home entry is in SETTINGS.homeOrder: its action, or 0x80 + the app's id for an app.
constexpr uint8_t appKey(const uint8_t appId) { return static_cast<uint8_t>(0x80 | appId); }
constexpr bool isAppKey(const uint8_t key) { return (key & 0x80) != 0; }

// The Home entries that are not apps, as their keys (HomeActivity's HomeMenuAction values), in Home's default order;
// the apps come between Saved and Settings.
enum EntryKey : uint8_t { KeyFiles = 0, KeyLibrary = 2, KeyOpds = 3, KeyStats = 4, KeySaved = 5, KeySettings = 8 };
struct EntryInfo {
  uint8_t key;
  StrId name;
  UIIcon icon;
};
constexpr int kEntryCount = 6;
extern const EntryInfo kEntries[kEntryCount];

// Whether an entry or app is left off the Home grid (Settings > Apps, or edit mode), and switching that.
bool entryHidden(uint8_t key);
void setEntryHidden(uint8_t key, bool hidden);

// The geometry of the Home pages.
struct Plan {
  int x = 0;  // the widgets' left edge and width
  int width = 0;
  int widgetTop = 0;
  int widgetRows = 0;
  int unitHeight = 0;  // one cell
  int columnWidth = 0;
  int gap = 0;
  // The app grid: columns, the rows on the first page and on the others, their tops and cell heights.
  int columns = 4;
  int firstRows = 0;
  int otherRows = 0;
  int firstTop = 0;
  int otherTop = 0;
  int firstCellHeight = 0;
  int otherCellHeight = 0;
  int cellWidth = 0;
  int gridLeft = 0;
  int plate = 0;     // the icon plate's side
  int iconSize = 32;
  int dotsY = 0;  // the page dots' band

  int firstCount() const { return columns * firstRows; }
  int otherCount() const { return columns * otherRows; }
  int pageCount(int entries) const;
  int pageOf(int entry) const;
  int firstOn(int page) const;
  Rect widgetRect(const Place& place, uint8_t size) const;
  Rect cellRect(int page, int slot) const;
};

// `greetingHeight` is the greeting's band above the widgets (0 when it is off).
Plan plan(const GfxRenderer& renderer, int greetingHeight);

// Touch ids (TouchRegistry::Cover) of the widgets outside edit mode.
constexpr int kTouchCurrentBook = 0;
constexpr int kTouchRecentBook = 10;  // + the book's index
constexpr int kTouchClock = 20;       // opens the Clock app (the clock, date and calendar widgets)
constexpr int kTouchStats = 21;       // opens the reading stats (the streak)
constexpr int kTouchUpNext = 22;      // opens the book that comes next
constexpr int kTouchSaved = 23;       // opens the saved highlights
constexpr int kTouchBattery = 24;     // opens Settings > Battery

// What the widgets show.
struct Data {
  const std::vector<RecentBook>* books = nullptr;
  const float* progress = nullptr;  // per recent book, 0-100 or negative
  int progressCount = 0;
  const BookReadingStats* stats = nullptr;  // the first book's, or nullptr
  float progressPercent = -1.0f;            // the first book's
  const GlobalReadingStats* global = nullptr;
  uint8_t weekStart = 0;  // clockface::WeekStart
  // Up next (empty title: nothing) and the highlight of the day (empty text: none), as Home found them.
  const std::string* upNextTitle = nullptr;
  const std::string* upNextDetail = nullptr;
  const std::string* highlightText = nullptr;
  const std::string* highlightSource = nullptr;
  bool selected = false;  // the current book is picked with the buttons
  bool editing = false;
  int picked = -1;  // the widget held in edit mode
};

// The parts that change rarely, kept in Home's snapshot: the cards and the covers.
void drawWidgetBackgrounds(GfxRenderer& renderer, const Plan& plan, const Data& data);
// Everything else, drawn over the snapshot on every pass, and the touch areas.
void drawWidgetContents(GfxRenderer& renderer, const Plan& plan, const Data& data);
// Whether a widget shows the time (Home then redraws every minute).
bool showsTime();

// The app grid of one page. `highlighted` is the entry drawn black (selected, pressed or held), -1 for none.
void drawGrid(GfxRenderer& renderer, const Plan& plan, int page, int entries, int highlighted,
              const std::function<const char*(int)>& label, const std::function<UIIcon(int)>& icon, bool editing);

// In edit mode: what is under a point of a page.
struct Hit {
  enum Kind : uint8_t { None, Widget, Entry } kind = None;
  int index = -1;
};
Hit hitTest(const Plan& plan, int page, int entries, int x, int y);

// Edit mode: the bars at the screen's sides an entry is dropped on to move it to the previous or next page.
constexpr int kPageEdge = 28;
void drawPageEdges(const GfxRenderer& renderer, const Plan& plan, int page, bool previous, bool next);

// The one-time introductions (bits of SETTINGS.homeIntroSeen): a full screen with its gestures and a button.
enum Intro : uint8_t { IntroHome = 1, IntroEdit = 2 };
void drawIntro(GfxRenderer& renderer, Intro intro);
Rect introButton(const GfxRenderer& renderer);

// Moves item `from` to where `to` is (both widgets or both entries); entries are written as their keys.
void moveWidget(int from, int to);
void moveEntry(const std::vector<uint8_t>& keys, int from, int to);

}  // namespace vegahome
