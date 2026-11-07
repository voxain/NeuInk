#pragma once

#include "RowListActivity.h"

// General > Software updates: the NeuInk logo large and the version installed; after an update check, the version it
// found (and "Update now" when that one is newer and was not installed). Under it: Check for updates, Switch to
// another firmware, SD card firmware update.
class SoftwareUpdatesActivity final : public RowListActivity {
 public:
  SoftwareUpdatesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

 protected:
  void buildRows(std::vector<Row>& rows) override;
  int headerHeight() const override;
  void buildHeader(UiApp::ScreenType& screen, const freeink::ui::Rect& area) override;
  void onHeaderAction(int value) override;

 private:
  void openSwitchFirmwarePicker();
  bool updateWaiting() const;
};
