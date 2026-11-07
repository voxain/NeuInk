#pragma once

#include "activities/Activity.h"

// Settings > Storage: how full the SD card is and what fills it, as one segmented bar and a row per category (see
// StorageScan). The page opens at once and shows "--" and a spinner on the bar until the scan, which takes a while on
// a full card, has run in the background.
class StorageActivity final : public Activity {
 public:
  StorageActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
