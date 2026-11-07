#pragma once

#include <memory>
#include <string>

#include "AppRegistry.h"
#include "GameUi.h"
#include "activities/Activity.h"

// The Apps screen (themes whose Home does not list the apps itself): a small tile
// per built-in app with its icon, its name and a short line about what is going
// on in it. Which apps show, and the Clock's and Sudoku's options, are in
// Settings > Apps (makeSettingsPage): an app that is switched off is only hidden
// from the tiles and from Vega's Home - it stays in the firmware and costs no
// memory or battery while it is not open.
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

  // Settings > Apps: everything with an icon on Home, each with a page: whether it shows on Home, and the app's
  // own options (the Clock's, Sudoku's).
  static std::unique_ptr<Activity> makeSettingsPage(GfxRenderer& renderer, MappedInputManager& mappedInput);

  // resumeId: one app's resume number opens that app on top of the list.
  AppsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, uint8_t resumeId = kResumeNone);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  using AppId = apps::Id;

  void layout();
  void readStatus();
  void open(const apps::Info& app);
  bool hidden(int app) const { return (hiddenMask & (1u << app)) != 0; }
  void drawTiles() const;

  uint8_t resumeId;
  uint8_t hiddenMask = 0;
  int shownCount = 0;
  int shown[apps::kIdCount] = {};
  gameui::Box tiles[apps::kIdCount];
  gameui::ButtonPointer pointer;
  std::string status[apps::kIdCount];
};
