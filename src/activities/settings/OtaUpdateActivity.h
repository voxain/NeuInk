#pragma once

#include "I18nKeys.h"
#include "activities/Activity.h"
#include "activities/ScreenTransitionRefresh.h"
#include "network/OtaUpdater.h"

class OtaUpdateActivity : public Activity {
  enum State {
    WIFI_SELECTION,
    CHECKING_FOR_UPDATE,
    WAITING_CONFIRMATION,
    UPDATE_IN_PROGRESS,
    NO_UPDATE,
    FAILED,
    FINISHED,
    SHUTTING_DOWN
  };

  // Can't initialize this to 0 or the first render doesn't happen
  static constexpr unsigned int UNINITIALIZED_PERCENTAGE = 111;

  State state = WIFI_SELECTION;
  ScreenTransitionRefresh screenTransitionRefresh;
  unsigned int lastUpdaterPercentage = UNINITIALIZED_PERCENTAGE;
  StrId failureMessage = StrId::STR_UPDATE_FAILED;
  OtaUpdater updater;
  // Set when the user accepted the "NeuInk only" warning; loop() then starts
  // the install on its own task instead of inside the popup's result callback.
  bool installAccepted = false;

  void onWifiSelectionComplete(bool success);
  void runUpdateInstall();
  void confirmNeuInkUpdate();
  void requestInstall();

 public:
  // `target` NEUINK is the normal update; any other is a cross-grade to that project.
  explicit OtaUpdateActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                             firmware_targets::Target target = firmware_targets::Target::NEUINK)
      : Activity("OtaUpdate", renderer, mappedInput), updater(target) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return state == CHECKING_FOR_UPDATE || state == UPDATE_IN_PROGRESS; }
  bool skipLoopDelay() override { return true; }  // Prevent power-saving mode
};
