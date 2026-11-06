#pragma once

#include <string>

#include "AppRegistry.h"
#include "ClockFace.h"
#include "GameUi.h"
#include "SudokuActivity.h"
#include "activities/Activity.h"

// The Apps screen (themes whose Home does not list the apps itself): a small tile
// per built-in app with its icon, its name and a short line about what is going
// on in it. Manage (top right) lists every app with a
// switch; an app that is switched off is only hidden from the tiles - it stays
// in the firmware and costs no memory or battery while it is not open. The
// Clock's and Sudoku's rows also lead to their settings.
class AppsActivity final : public Activity {
 public:
  // What sleep notes about the screen it began on, so that waking returns there
  // the way it returns to a book: nothing, this list, or one app (numbers above
  // kResumeList). They are kept in the state file and must not change.
  static constexpr uint8_t kResumeNone = 0;
  static constexpr uint8_t kResumeList = 1;
  // The resume number of the screen with this activity name; kResumeNone if it
  // is neither the list nor an app.
  static uint8_t resumeIdFor(const std::string& activityName);

  // resumeId: one app's resume number opens that app on top of the list.
  // manageOnly: opened from Settings, straight on the Manage page, and Back leaves the screen.
  AppsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, uint8_t resumeId = kResumeNone,
               bool manageOnly = false);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  using AppId = apps::Id;

  void layout();
  void readStatus();
  void open(const apps::Info& app);
  bool hidden(int app) const { return (hiddenMask & (1u << app)) != 0; }
  void saveHidden() const;
  void drawTiles() const;
  void drawManage() const;
  void drawClockSettings() const;
  void changeClockSetting(int row);
  void drawSudokuSettings() const;
  // The Settings button on an app's row in Manage, left of its switch.
  gameui::Box settingsButtonFor(AppId app) const;

  enum class Mode : uint8_t { Tiles, Manage, ClockSettings, SudokuSettings };
  enum ClockRow { FaceRow = 0, PersianRow = 1, WeekStartRow = 2, WeekNumberRow = 3, kClockRowCount = 4 };
  // The Sudoku switches fill two pages.
  static constexpr int kSudokuRowsPerPage = 5;
  static constexpr int kSudokuPages =
      (SudokuActivity::kOptionCount + kSudokuRowsPerPage - 1) / kSudokuRowsPerPage;

  Mode mode = Mode::Tiles;
  uint8_t resumeId;
  bool manageOnly;
  clockface::Options clockOptions;
  gameui::Box clockSettingsBox;
  gameui::Box clockRows[kClockRowCount];
  SudokuActivity::Options sudokuOptions;
  int sudokuPage = 0;
  gameui::Box sudokuSettingsBox;
  gameui::Box sudokuRows[kSudokuRowsPerPage];
  gameui::Box pageBoxes[2];  // previous, next
  uint8_t hiddenMask = 0;
  int shownCount = 0;
  int shown[apps::kIdCount] = {};
  gameui::Box tiles[apps::kIdCount];
  gameui::Box rows[apps::kIdCount];
  gameui::Box manageBox;
  gameui::ButtonPointer pointer;
  std::string status[apps::kIdCount];
};
