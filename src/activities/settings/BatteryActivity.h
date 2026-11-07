#pragma once

#include "activities/Activity.h"
#include "util/BatteryHistory.h"

#if NEUINK_BATTERY_HISTORY
// Settings > Battery: the charge level now and as a graph of the last 24 hours (see BatteryHistory). Reading the
// graph costs nothing: it is the stored history, drawn once.
class BatteryActivity final : public Activity {
 public:
  BatteryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  // The apps list: the first row shown, how many fit, whether more follow.
  int firstRow = 0;
  int shownRows = 0;
  bool moreBelow = false;
};
#endif
