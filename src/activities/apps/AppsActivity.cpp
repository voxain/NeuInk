#include "AppsActivity.h"

#include <I18n.h>

#include <cstdio>
#include <memory>

#include "CheckersActivity.h"
#include "ChessActivity.h"
#include "ClockActivity.h"
#include "GoActivity.h"
#include "MappedInputManager.h"
#include "SudokuActivity.h"
#include "TicTacToeActivity.h"
#include "AppCapabilities.h"
#include "OpdsServerStore.h"
#include "activities/ActivityManager.h"
#include "activities/home/HighlightsHubActivity.h"
#include "activities/home/StatsPickerActivity.h"
#include "activities/settings/RowListActivity.h"
#include "components/UiAppHelpers.h"
#include "components/icons/homeIconSet.h"
#include "components/themes/vega/VegaHome.h"
#include "fontIds.h"

namespace {

constexpr int kColumns = 3;
constexpr int kTileMargin = 16;
constexpr int kTileGap = 12;
// Small tiles: the 32 px icon, the name and one status line.
constexpr int kTileHeight = 112;
constexpr int kIconSize = 32;
constexpr int kIconTop = 14;
constexpr int kNameTop = 54;
constexpr int kStatusTop = 82;

}  // namespace

AppsActivity::AppsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const uint8_t resumeId)
    : Activity("Apps", renderer, mappedInput), resumeId(resumeId) {}

uint8_t AppsActivity::resumeIdFor(const std::string& activityName) {
  if (activityName == "Apps") return kResumeList;
  const apps::Info* app = apps::findByActivityName(activityName);
  return app ? static_cast<uint8_t>(kResumeList + 1 + app->id) : kResumeNone;
}

void AppsActivity::onEnter() {
  Activity::onEnter();
  pointer.begin(mappedInput);
  hiddenMask = apps::hiddenMask();

  // Woken from a sleep that began inside an app: straight back into it. The
  // tiles are read and drawn when that app is left.
  const int resumeApp = static_cast<int>(resumeId) - (kResumeList + 1);
  resumeId = kResumeNone;
  if (const apps::Info* app = apps::find(static_cast<AppId>(resumeApp)); app && !hidden(app->id)) {
    open(*app);
    return;
  }

  readStatus();
  requestUpdate();
}

void AppsActivity::readStatus() {
  char line[96];

  {
    // Larger than the main task's stack should carry next to the lines above.
    const auto sudokuGame = std::make_unique<sudoku::Game>();
    SudokuActivity::readGame(*sudokuGame);
    if (sudokuGame->started() && !sudokuGame->solved()) {
      snprintf(line, sizeof(line), tr(STR_SUDOKU_IN_PROGRESS),
               sudokuGame->dailyDay() != 0 ? tr(STR_SUDOKU_DAILY)
                                           : SudokuActivity::levelName(sudokuGame->difficulty()),
               sudokuGame->emptyCells());
    } else {
      const unsigned solved = sudokuGame->totalSolved();
      if (solved > 0) {
        snprintf(line, sizeof(line), tr(STR_SUDOKU_RECORD), solved);
      } else {
        line[0] = '\0';
      }
    }
    status[apps::Sudoku] = line;
  }

  ChessActivity::statusLine(line, sizeof(line));
  status[apps::Chess] = line;
  CheckersActivity::statusLine(line, sizeof(line));
  status[apps::Checkers] = line;
  ClockActivity::statusLine(line, sizeof(line));
  status[apps::Clock] = line;
  TicTacToeActivity::statusLine(line, sizeof(line));
  status[apps::TicTacToe] = line;
  GoActivity::statusLine(line, sizeof(line));
  status[apps::Go] = line;
}

void AppsActivity::layout() {
  const int width = renderer.getScreenWidth();
  const int top = gameui::contentTop(renderer, mappedInput);
  const int tileWidth = (width - 2 * kTileMargin - (kColumns - 1) * kTileGap) / kColumns;
  shownCount = 0;
  for (const apps::Info& app : apps::kAll) {
    if (hidden(app.id)) continue;
    tiles[shownCount] = {kTileMargin + (shownCount % kColumns) * (tileWidth + kTileGap),
                         top + (shownCount / kColumns) * (kTileHeight + kTileGap), tileWidth, kTileHeight};
    shown[shownCount++] = app.id;
  }
}

void AppsActivity::open(const apps::Info& app) {
  startActivityForResult(app.create(renderer, mappedInput), [this](const ActivityResult&) {
    readStatus();
    requestUpdate();
  });
}

void AppsActivity::loop() {
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    setResult(ActivityResult{});
    finish();
    return;
  }

  int tx = 0;
  int ty = 0;
  bool moved = false;
  bool hit;
  {
    RenderLock lock(*this);
    hit = pointer.tapped(mappedInput, tx, ty, moved);
  }
  if (moved) requestUpdate();
  if (!hit) return;
  for (int i = 0; i < shownCount; ++i) {
    if (tiles[i].contains(tx, ty)) {
      open(*apps::find(static_cast<AppId>(shown[i])));
      return;
    }
  }
}

void AppsActivity::drawTiles() const {
  for (int i = 0; i < shownCount; ++i) {
    const apps::Info& app = *apps::find(static_cast<AppId>(shown[i]));
    const gameui::Box& tile = tiles[i];
    renderer.drawRoundedRect(tile.x, tile.y, tile.w, tile.h, 2, 12, true);
    if (const freeink::Icon* icon = homeIcon(app.icon, kIconSize)) {
      drawLucideIcon(renderer, *icon, tile.x + (tile.w - kIconSize) / 2, tile.y + kIconTop);
    }
    gameui::centredText(renderer, UI_10_FONT_ID, {tile.x, tile.y + kNameTop, tile.w, 26},
                        renderer.truncatedText(UI_10_FONT_ID, I18N.get(app.name), tile.w - 12, EpdFontFamily::BOLD).c_str(),
                        true, EpdFontFamily::BOLD);
    if (!status[app.id].empty()) {
      gameui::centredText(renderer, SMALL_FONT_ID, {tile.x, tile.y + kStatusTop, tile.w, 22},
                          renderer.truncatedText(SMALL_FONT_ID, status[app.id].c_str(), tile.w - 12).c_str());
    }
  }
  if (shownCount == 0) {
    gameui::centredText(renderer, UI_10_FONT_ID, {0, gameui::contentTop(renderer, mappedInput) + 40, renderer.getScreenWidth(), 30},
                        tr(STR_APPS_ALL_HIDDEN));
  }
}

void AppsActivity::render(RenderLock&&) {
  layout();
  renderer.clearScreen();
  gameui::drawHeader(renderer, mappedInput, tr(STR_APPS));
  drawTiles();
  pointer.clear();
  for (int i = 0; i < shownCount; ++i) pointer.add(tiles[i]);
  pointer.draw(renderer);
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}

// ---- Settings > Apps -------------------------------------------------------------------------------------

namespace {

using Row = RowListActivity::Row;

Row toggleRow(std::string label, const bool checked, std::function<void()> onTap, std::string subtitle = {}) {
  Row row;
  row.kind = Row::Kind::Toggle;
  row.label = std::move(label);
  row.checked = checked;
  row.onTap = std::move(onTap);
  row.subtitle = std::move(subtitle);
  return row;
}

Row valueRow(std::string label, std::string value, std::function<void()> onTap) {
  Row row;
  row.label = std::move(label);
  row.value = std::move(value);
  row.onTap = std::move(onTap);
  return row;
}

// The Clock's options, under its switch on its page in Settings > Apps.
void addClockRows(std::vector<Row>& rows) {
  clockface::Options options;
  clockface::loadOptions(options);
  const auto change = [](const std::function<void(clockface::Options&)>& edit) {
    clockface::Options current;
    clockface::loadOptions(current);
    edit(current);
    clockface::saveOptions(current);
  };
  const char* faces[clockface::kFaceCount] = {tr(STR_CLOCK_FACE_DIGITAL), tr(STR_CLOCK_FACE_ANALOG),
                                              tr(STR_CLOCK_FACE_PERSIAN)};
  // The weekdays are numbered from Monday.
  const int firstDay = options.weekStart == clockface::Saturday ? 5 : (options.weekStart == clockface::Sunday ? 6 : 0);
  rows.push_back(valueRow(tr(STR_CLOCK_FACE), faces[options.shownFace()], [change] {
    change([](clockface::Options& o) {
      // The face with Persian days needs the Persian date on.
      do {
        o.face = static_cast<uint8_t>((o.face + 1) % clockface::kFaceCount);
      } while (o.face == clockface::PersianDays && !o.persianDate);
    });
  }));
  rows.push_back(toggleRow(tr(STR_CLOCK_PERSIAN_DATE), options.persianDate, [change] {
    change([](clockface::Options& o) {
      o.persianDate = !o.persianDate;
      if (!o.persianDate && o.face == clockface::PersianDays) o.face = clockface::Digital;
    });
  }));
  rows.push_back(valueRow(tr(STR_CLOCK_WEEK_START), clockface::weekdayName(firstDay), [change] {
    change([](clockface::Options& o) { o.weekStart = static_cast<uint8_t>((o.weekStart + 1) % clockface::kWeekStartCount); });
  }));
  rows.push_back(toggleRow(tr(STR_CLOCK_WEEK_NUMBER), options.weekNumber, [change] {
    change([](clockface::Options& o) { o.weekNumber = !o.weekNumber; });
  }, tr(STR_CLOCK_FORMAT_NOTE)));
}

// Sudoku's options, likewise.
void addSudokuRows(std::vector<Row>& rows) {
  SudokuActivity::Options options;
  SudokuActivity::loadOptions(options);
  const char* labels[SudokuActivity::kOptionCount] = {
      tr(STR_SUDOKU_OPT_EXPLAIN), tr(STR_SUDOKU_OPT_DAILY),     tr(STR_SUDOKU_OPT_LIVE),
      tr(STR_SUDOKU_OPT_REPEATS), tr(STR_SUDOKU_OPT_PAINT),     tr(STR_SUDOKU_OPT_FILL_LAST),
      tr(STR_SUDOKU_OPT_LIMIT),   tr(STR_SUDOKU_OPT_RESTART),   tr(STR_SUDOKU_OPT_STATS)};
  const char* descriptions[SudokuActivity::kOptionCount] = {
      tr(STR_SUDOKU_OPT_EXPLAIN_DESC), tr(STR_SUDOKU_OPT_DAILY_DESC),     tr(STR_SUDOKU_OPT_LIVE_DESC),
      tr(STR_SUDOKU_OPT_REPEATS_DESC), tr(STR_SUDOKU_OPT_PAINT_DESC),     tr(STR_SUDOKU_OPT_FILL_LAST_DESC),
      tr(STR_SUDOKU_OPT_LIMIT_DESC),   tr(STR_SUDOKU_OPT_RESTART_DESC),   tr(STR_SUDOKU_OPT_STATS_DESC)};
  for (int i = 0; i < SudokuActivity::kOptionCount; ++i) {
    rows.push_back(toggleRow(labels[i], options.on[i], [i] {
      SudokuActivity::Options current;
      SudokuActivity::loadOptions(current);
      current.on[i] = !current.on[i];
      SudokuActivity::saveOptions(current);
    }, descriptions[i]));
  }
}

// The File Manager's options: whether plugging into a computer offers the SD card as a USB drive.
void addFilesRows(std::vector<Row>& rows) {
#if CROSSINK_APP_CAP_USB_DRIVE
  rows.push_back(toggleRow(tr(STR_USB_DRIVE_PROMPT), SETTINGS.usbDrivePrompt != 0, [] {
    SETTINGS.usbDrivePrompt = SETTINGS.usbDrivePrompt ? 0 : 1;
    SETTINGS.saveToFile();
  }, tr(STR_USB_DRIVE_PROMPT_DESC)));
#else
  (void)rows;
#endif
}

// One app's page: its icon large, Open and Enable / Disable (whether it shows on Home), a rule, then the app's own
// options, or a gray note when it has none.
class AppPageActivity final : public RowListActivity {
 public:
  AppPageActivity(GfxRenderer& renderer, MappedInputManager& input, const uint8_t key, const char* name,
                  const UIIcon icon)
      : RowListActivity(renderer, input, "AppSettings", name), key(key), icon(icon) {}

 protected:
  void buildRows(std::vector<Row>& rows) override {
    rows.clear();
    if (key == vegahome::appKey(apps::Clock)) addClockRows(rows);
    if (key == vegahome::appKey(apps::Sudoku)) addSudokuRows(rows);
    if (key == vegahome::KeyFiles) addFilesRows(rows);
  }

  int headerHeight() const override { return kPlate + 20 + kButtonH + 22 + 2 + 10; }

  void buildHeader(UiApp::ScreenType& screen, const freeink::ui::Rect& area) override {
    namespace fui = freeink::ui;
    const int cx = area.x + area.width / 2;
    fui::fillPlate(screen.target(),
                   fui::Rect{static_cast<int16_t>(cx - kPlate / 2), area.y, kPlate, kPlate},
                   static_cast<fui::PlateShape>(SETTINGS.homeIconShape), fui::Paint::dither(fui::Color::LightGray));
    if (const freeink::Icon* glyph = homeIcon(icon, 48)) {
      drawLucideIcon(renderer, *glyph, cx - 24, area.y + (kPlate - 48) / 2);
    }
    // Open (not for Settings, which is open already) and Enable / Disable, side by side.
    const bool hidden = vegahome::entryHidden(key);
    const bool canOpen = key != vegahome::KeySettings;
    const int16_t buttonsY = static_cast<int16_t>(area.y + kPlate + 20);
    const int16_t buttonW = static_cast<int16_t>((area.width - 3 * kSide) / 2);
    int16_t x = static_cast<int16_t>(canOpen ? area.x + kSide : cx - buttonW / 2);
    const auto button = [&](const freeink::Icon& glyph, const char* label, const int value) {
      fui::ButtonProps props;
      props.label = label;
      props.icon = fui::bitmapFromIcon(glyph);
      props.action = ACTION_HEADER;
      props.value = static_cast<int16_t>(value);
      props.styles = grayButtonStyles();
      props.radius = kButtonH / 2;
      props.text = screen.theme().bodyText;
      props.gap = 10;
      fui::button(screen.frame(), fui::Rect{x, buttonsY, buttonW, kButtonH}, props);
      x = static_cast<int16_t>(x + buttonW + kSide);
    };
    if (canOpen) button(icon_external_link_24, tr(STR_OPEN), kOpen);
    button(hidden ? icon_eye_24 : icon_eye_off_24, hidden ? tr(STR_ENABLE) : tr(STR_DISABLE), kToggle);
    // The rule under the buttons, gray.
    renderer.fillRectDither(area.x + kSide, buttonsY + kButtonH + 22, area.width - 2 * kSide, 2, Color::DarkGray);
  }

  void onHeaderAction(const int value) override {
    if (value == kToggle) {
      vegahome::setEntryHidden(key, !vegahome::entryHidden(key));
      return;
    }
    if (vegahome::isAppKey(key)) {
      if (const apps::Info* app = apps::find(static_cast<apps::Id>(key & 0x7F))) openChild(app->create(renderer, mappedInput));
      return;
    }
    switch (key) {
      case vegahome::KeyFiles:
        activityManager.goToFileBrowser();
        break;
      case vegahome::KeyLibrary:
        activityManager.goToLibrary();
        break;
      case vegahome::KeyOpds:
        activityManager.goToBrowser();
        break;
      case vegahome::KeyStats:
        openChild(std::make_unique<StatsPickerActivity>(renderer, mappedInput));
        break;
      case vegahome::KeySaved:
        openChild(std::make_unique<HighlightsHubActivity>(renderer, mappedInput));
        break;
      default:
        break;
    }
  }

  // Settings has no options of its own; its page says where Settings stay when its icon is hidden.
  const char* emptyText() const override {
    return key == vegahome::KeySettings ? tr(STR_SETTINGS_HIDDEN_NOTE) : tr(STR_NO_APP_SETTINGS);
  }

 private:
  static constexpr int16_t kPlate = 121;  // odd, as fillPlate wants
  static constexpr int16_t kButtonH = 52;
  static constexpr int kSide = 16;
  static constexpr int kOpen = 0;
  static constexpr int kToggle = 1;
  uint8_t key;
  UIIcon icon;
};

}  // namespace

std::unique_ptr<Activity> AppsActivity::makeSettingsPage(GfxRenderer& renderer, MappedInputManager& input) {
  return std::make_unique<LambdaRowList>(
      renderer, input, "AppsSettings", tr(STR_APPS), [&renderer, &input](std::vector<Row>& rows, RowListActivity& page) {
        // Everything with an icon on Home, in Home's default order: the entries, the apps among them before Settings.
        rows.clear();
        const auto add = [&](const uint8_t key, const StrId name, const UIIcon icon) {
          Row row;
          row.kind = Row::Kind::Chevron;
          row.label = I18N.get(name);
          row.icon = static_cast<int8_t>(icon);
          if (vegahome::entryHidden(key)) {
            row.subtitle = tr(STR_HIDDEN_ON_HOME);
          } else if (key == vegahome::KeyOpds && !OPDS_STORE.hasServers()) {
            // The OPDS browser joins Home once there is a catalog to browse.
            row.subtitle = tr(STR_OPDS_NEEDS_SERVER);
          }
          row.onTap = [&renderer, &input, &page, key, name, icon] {
            page.openChild(std::make_unique<AppPageActivity>(renderer, input, key, I18N.get(name), icon));
          };
          rows.push_back(std::move(row));
        };
        for (const vegahome::EntryInfo& entry : vegahome::kEntries) {
          if (entry.key == vegahome::KeySettings) {
            for (const apps::Info& app : apps::kAll) add(vegahome::appKey(app.id), app.name, app.icon);
          }
          add(entry.key, entry.name, entry.icon);
        }
      });
}
