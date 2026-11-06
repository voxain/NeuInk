#include "AppsActivity.h"

#include <I18n.h>

#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <memory>

#include "CheckersActivity.h"
#include "ChessActivity.h"
#include "ClockActivity.h"
#include "GoActivity.h"
#include "MappedInputManager.h"
#include "SudokuActivity.h"
#include "TicTacToeActivity.h"
#include "components/UiAppHelpers.h"
#include "components/icons/homeIconSet.h"
#include "fontIds.h"

namespace {

constexpr int kColumns = 3;
constexpr int kTileMargin = 16;
constexpr int kTileGap = 12;
// Small tiles: the 48 px icon, the name and one status line.
constexpr int kTileHeight = 128;
constexpr int kIconSize = 48;  // Lucide's grid at 2x: every stroke on whole pixels
constexpr int kIconTop = 14;
constexpr int kNameTop = 72;
constexpr int kStatusTop = 100;
constexpr int kRowHeight = 72;
constexpr int kRowGap = 12;

void drawSwitch(const GfxRenderer& renderer, const int x, const int y, const bool on) {
  constexpr int width = 66;
  constexpr int height = 36;
  if (on) {
    renderer.fillRoundedRect(x, y, width, height, height / 2, Color::Black);
    gameui::disc(renderer, x + width - height / 2, y + height / 2, 13, false);
  } else {
    renderer.drawRoundedRect(x, y, width, height, 2, height / 2, true);
    gameui::disc(renderer, x + height / 2, y + height / 2, 12, true);
  }
}

}  // namespace

AppsActivity::AppsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const uint8_t resumeId,
                           const bool manageOnly)
    : Activity("Apps", renderer, mappedInput), resumeId(resumeId), manageOnly(manageOnly) {}

uint8_t AppsActivity::resumeIdFor(const std::string& activityName) {
  if (activityName == "Apps") return kResumeList;
  const apps::Info* app = apps::findByActivityName(activityName);
  return app ? static_cast<uint8_t>(kResumeList + 1 + app->id) : kResumeNone;
}

void AppsActivity::onEnter() {
  Activity::onEnter();
  pointer.begin(mappedInput);
  hiddenMask = apps::hiddenMask();
  mode = manageOnly ? Mode::Manage : Mode::Tiles;
  clockface::loadOptions(clockOptions);
  SudokuActivity::loadOptions(sudokuOptions);

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

void AppsActivity::saveHidden() const { apps::saveHiddenMask(hiddenMask); }

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
  int row = 0;
  for (const apps::Info& app : apps::kAll) {
    rows[app.id] = {kTileMargin, top + row++ * (kRowHeight + kRowGap), width - 2 * kTileMargin, kRowHeight};
    if (hidden(app.id)) continue;
    tiles[shownCount] = {kTileMargin + (shownCount % kColumns) * (tileWidth + kTileGap),
                         top + (shownCount / kColumns) * (kTileHeight + kTileGap), tileWidth, kTileHeight};
    shown[shownCount++] = app.id;
  }
  manageBox = gameui::headerCornerBox(renderer, mappedInput, tr(STR_APPS_MANAGE));

  clockSettingsBox = settingsButtonFor(apps::Clock);
  sudokuSettingsBox = settingsButtonFor(apps::Sudoku);
  for (int row = 0; row < kClockRowCount; ++row) {
    clockRows[row] = {kTileMargin, top + row * (84 + kRowGap), width - 2 * kTileMargin, 84};
  }
  for (int row = 0; row < kSudokuRowsPerPage; ++row) {
    sudokuRows[row] = {kTileMargin, top + row * (84 + kRowGap), width - 2 * kTileMargin, 84};
  }
  // Previous and next page at the foot, with "1 / 2" between them. Wide, so the
  // button pointer goes from "<" to ">" rather than up to the last row.
  const int pageY = gameui::contentBottom(renderer) - 56;
  pageBoxes[0] = {kTileMargin, pageY, 160, 56};
  pageBoxes[1] = {width - kTileMargin - 160, pageY, 160, 56};
}

gameui::Box AppsActivity::settingsButtonFor(const AppId app) const {
  const gameui::Box& row = rows[app];
  const int settingsWidth = renderer.getTextWidth(UI_10_FONT_ID, tr(STR_SETTINGS_SHORT), EpdFontFamily::BOLD) + 40;
  return {row.x + row.w - 66 - 22 - 14 - settingsWidth, row.y + (row.h - 50) / 2, settingsWidth, 50};
}

void AppsActivity::changeClockSetting(const int row) {
  switch (row) {
    case FaceRow:
      // The face with Persian days needs the Persian date on.
      do {
        clockOptions.face = static_cast<uint8_t>((clockOptions.face + 1) % clockface::kFaceCount);
      } while (clockOptions.face == clockface::PersianDays && !clockOptions.persianDate);
      break;
    case PersianRow:
      clockOptions.persianDate = !clockOptions.persianDate;
      if (!clockOptions.persianDate && clockOptions.face == clockface::PersianDays) {
        clockOptions.face = clockface::Digital;
      }
      break;
    case WeekStartRow:
      clockOptions.weekStart = static_cast<uint8_t>((clockOptions.weekStart + 1) % clockface::kWeekStartCount);
      break;
    default:
      clockOptions.weekNumber = !clockOptions.weekNumber;
      break;
  }
  clockface::saveOptions(clockOptions);
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
    if (mode != Mode::Tiles && !(manageOnly && mode == Mode::Manage)) {
      // One step back: from an app's settings to the switches, from there to the tiles.
      mode = mode == Mode::Manage ? Mode::Tiles : Mode::Manage;
      requestUpdate();
      return;
    }
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
  if (mode == Mode::ClockSettings) {
    for (int row = 0; row < kClockRowCount; ++row) {
      if (!clockRows[row].contains(tx, ty)) continue;
      changeClockSetting(row);
      requestUpdate();
      return;
    }
    return;
  }
  if (mode == Mode::SudokuSettings) {
    for (int i = 0; i < 2; ++i) {
      if (!pageBoxes[i].contains(tx, ty)) continue;
      sudokuPage = (sudokuPage + (i == 0 ? kSudokuPages - 1 : 1)) % kSudokuPages;
      requestUpdate();
      return;
    }
    for (int row = 0; row < kSudokuRowsPerPage; ++row) {
      const int option = sudokuPage * kSudokuRowsPerPage + row;
      if (option >= SudokuActivity::kOptionCount || !sudokuRows[row].contains(tx, ty)) continue;
      sudokuOptions.on[option] = !sudokuOptions.on[option];
      SudokuActivity::saveOptions(sudokuOptions);
      requestUpdate();
      return;
    }
    return;
  }
  if (mode == Mode::Manage) {
    if (clockSettingsBox.grown(6).contains(tx, ty)) {
      mode = Mode::ClockSettings;
      requestUpdate();
      return;
    }
    if (sudokuSettingsBox.grown(6).contains(tx, ty)) {
      mode = Mode::SudokuSettings;
      sudokuPage = 0;
      requestUpdate();
      return;
    }
    for (const apps::Info& app : apps::kAll) {
      if (!rows[app.id].contains(tx, ty)) continue;
      hiddenMask ^= static_cast<uint8_t>(1u << app.id);
      saveHidden();
      requestUpdate();
      return;
    }
    return;
  }
  if (manageBox.grown(6).contains(tx, ty)) {
    mode = Mode::Manage;
    requestUpdate();
    return;
  }
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
    gameui::centredText(renderer, UI_10_FONT_ID, {0, tiles[0].y + 40, renderer.getScreenWidth(), 30},
                        tr(STR_APPS_ALL_HIDDEN));
  }
}

void AppsActivity::drawManage() const {
  for (const apps::Info& app : apps::kAll) {
    const gameui::Box& row = rows[app.id];
    renderer.drawRoundedRect(row.x, row.y, row.w, row.h, 2, 12, true);
    const gameui::Box label{row.x + 24, row.y + 6, 190, row.h - 12};
    renderer.drawText(UI_12_FONT_ID, label.x, gameui::centredTextY(renderer, UI_12_FONT_ID, label), I18N.get(app.name), true,
                      EpdFontFamily::BOLD);
    if (hidden(app.id)) gameui::fade(renderer, label);
    drawSwitch(renderer, row.x + row.w - 66 - 22, row.y + (row.h - 36) / 2, !hidden(app.id));
    if (app.id == apps::Clock) gameui::button(renderer, clockSettingsBox, tr(STR_SETTINGS_SHORT), false);
    if (app.id == apps::Sudoku) gameui::button(renderer, sudokuSettingsBox, tr(STR_SETTINGS_SHORT), false);
  }
  const int y = rows[apps::kAll[apps::kCount - 1].id].y + kRowHeight + 14;
  int lineY = y;
  for (const auto& line : renderer.wrappedText(SMALL_FONT_ID, tr(STR_APPS_HIDDEN_NOTE),
                                               renderer.getScreenWidth() - 60, 3)) {
    gameui::centredText(renderer, SMALL_FONT_ID, {0, lineY, renderer.getScreenWidth(), 24}, line.c_str());
    lineY += 24;
  }
}

void AppsActivity::drawClockSettings() const {
  const char* faces[clockface::kFaceCount] = {tr(STR_CLOCK_FACE_DIGITAL), tr(STR_CLOCK_FACE_ANALOG),
                                              tr(STR_CLOCK_FACE_PERSIAN)};
  // The weekdays are numbered from Monday.
  const int firstDay = clockOptions.weekStart == clockface::Saturday
                           ? 5
                           : (clockOptions.weekStart == clockface::Sunday ? 6 : 0);
  const char* labels[kClockRowCount] = {tr(STR_CLOCK_FACE), tr(STR_CLOCK_PERSIAN_DATE), tr(STR_CLOCK_WEEK_START),
                                        tr(STR_CLOCK_WEEK_NUMBER)};
  const char* values[kClockRowCount] = {faces[clockOptions.shownFace()],
                                        clockOptions.persianDate ? tr(STR_STATE_ON) : tr(STR_STATE_OFF),
                                        clockface::weekdayName(firstDay),
                                        clockOptions.weekNumber ? tr(STR_STATE_ON) : tr(STR_STATE_OFF)};
  for (int row = 0; row < kClockRowCount; ++row) {
    const gameui::Box& box = clockRows[row];
    renderer.drawRoundedRect(box.x, box.y, box.w, box.h, 2, 12, true);
    renderer.drawText(UI_12_FONT_ID, box.x + 24, gameui::centredTextY(renderer, UI_12_FONT_ID, box), labels[row], true,
                      EpdFontFamily::BOLD);
    renderer.drawText(UI_10_FONT_ID, box.x + box.w - 24 - renderer.getTextWidth(UI_10_FONT_ID, values[row]),
                      gameui::centredTextY(renderer, UI_10_FONT_ID, box), values[row]);
  }
  int y = clockRows[kClockRowCount - 1].y + 84 + 14;
  for (const char* note : {tr(STR_CLOCK_TAP_ROW), tr(STR_CLOCK_FORMAT_NOTE)}) {
    for (const auto& line : renderer.wrappedText(SMALL_FONT_ID, note, renderer.getScreenWidth() - 60, 3)) {
      gameui::centredText(renderer, SMALL_FONT_ID, {0, y, renderer.getScreenWidth(), 24}, line.c_str());
      y += 24;
    }
  }
}

void AppsActivity::drawSudokuSettings() const {
  const char* labels[SudokuActivity::kOptionCount] = {
      tr(STR_SUDOKU_OPT_EXPLAIN), tr(STR_SUDOKU_OPT_DAILY),     tr(STR_SUDOKU_OPT_LIVE),
      tr(STR_SUDOKU_OPT_REPEATS), tr(STR_SUDOKU_OPT_PAINT),     tr(STR_SUDOKU_OPT_FILL_LAST),
      tr(STR_SUDOKU_OPT_LIMIT),   tr(STR_SUDOKU_OPT_RESTART),   tr(STR_SUDOKU_OPT_STATS)};
  const char* descriptions[SudokuActivity::kOptionCount] = {
      tr(STR_SUDOKU_OPT_EXPLAIN_DESC), tr(STR_SUDOKU_OPT_DAILY_DESC),     tr(STR_SUDOKU_OPT_LIVE_DESC),
      tr(STR_SUDOKU_OPT_REPEATS_DESC), tr(STR_SUDOKU_OPT_PAINT_DESC),     tr(STR_SUDOKU_OPT_FILL_LAST_DESC),
      tr(STR_SUDOKU_OPT_LIMIT_DESC),   tr(STR_SUDOKU_OPT_RESTART_DESC),   tr(STR_SUDOKU_OPT_STATS_DESC)};
  int bottom = sudokuRows[0].y;
  for (int row = 0; row < kSudokuRowsPerPage; ++row) {
    const int option = sudokuPage * kSudokuRowsPerPage + row;
    if (option >= SudokuActivity::kOptionCount) break;
    const gameui::Box& box = sudokuRows[row];
    renderer.drawRoundedRect(box.x, box.y, box.w, box.h, 2, 12, true);
    // The name and, under it, one line on what the switch does.
    const int textWidth = box.w - 24 - 66 - 22 - 12;
    renderer.drawText(UI_12_FONT_ID, box.x + 24, gameui::centredTextY(renderer, UI_12_FONT_ID, {box.x, box.y + 8, box.w, 36}),
                      renderer.truncatedText(UI_12_FONT_ID, labels[option], textWidth, EpdFontFamily::BOLD).c_str(),
                      true, EpdFontFamily::BOLD);
    renderer.drawText(SMALL_FONT_ID, box.x + 24,
                      gameui::centredTextY(renderer, SMALL_FONT_ID, {box.x, box.y + 46, box.w, 28}),
                      renderer.truncatedText(SMALL_FONT_ID, descriptions[option], textWidth).c_str());
    drawSwitch(renderer, box.x + box.w - 66 - 22, box.y + (box.h - 36) / 2, sudokuOptions.on[option]);
    bottom = box.y + box.h;
  }
  int y = bottom + 14;
  for (const auto& line : renderer.wrappedText(SMALL_FONT_ID, tr(STR_CLOCK_TAP_ROW), renderer.getScreenWidth() - 60, 2)) {
    gameui::centredText(renderer, SMALL_FONT_ID, {0, y, renderer.getScreenWidth(), 24}, line.c_str());
    y += 24;
  }
  gameui::button(renderer, pageBoxes[0], "<", false, UI_12_FONT_ID);
  gameui::button(renderer, pageBoxes[1], ">", false, UI_12_FONT_ID);
  char page[16];
  snprintf(page, sizeof(page), "%d / %d", sudokuPage + 1, kSudokuPages);
  gameui::centredText(renderer, UI_12_FONT_ID, {0, pageBoxes[0].y, renderer.getScreenWidth(), pageBoxes[0].h}, page,
                      true, EpdFontFamily::BOLD);
}

void AppsActivity::render(RenderLock&&) {
  layout();
  renderer.clearScreen();
  const char* title = mode == Mode::ClockSettings    ? tr(STR_CLOCK_SETTINGS)
                      : mode == Mode::SudokuSettings ? tr(STR_SUDOKU_SETTINGS)
                      : mode == Mode::Manage         ? tr(STR_APPS_MANAGE_TITLE)
                                                     : tr(STR_APPS);
  gameui::drawHeader(renderer, mappedInput, title);
  if (mode == Mode::ClockSettings) {
    drawClockSettings();
  } else if (mode == Mode::SudokuSettings) {
    drawSudokuSettings();
  } else if (mode == Mode::Manage) {
    drawManage();
  } else {
    gameui::cornerButton(renderer, manageBox, tr(STR_APPS_MANAGE));
    drawTiles();
  }
  pointer.clear();
  if (mode == Mode::ClockSettings) {
    for (const auto& row : clockRows) pointer.add(row);
  } else if (mode == Mode::SudokuSettings) {
    for (int row = 0; row < kSudokuRowsPerPage; ++row) {
      if (sudokuPage * kSudokuRowsPerPage + row < SudokuActivity::kOptionCount) pointer.add(sudokuRows[row]);
    }
    for (const auto& box : pageBoxes) pointer.add(box);
  } else if (mode == Mode::Manage) {
    for (const auto& row : rows) pointer.add(row);
    pointer.add(clockSettingsBox);
    pointer.add(sudokuSettingsBox);
  } else {
    for (int i = 0; i < shownCount; ++i) pointer.add(tiles[i]);
    pointer.add(manageBox);
  }
  pointer.draw(renderer);
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
