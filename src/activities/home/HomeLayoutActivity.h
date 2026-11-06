#pragma once

#include "components/PanelActivity.h"

// The Vega Home grid's layout, a panel over Home that a swipe up from the bottom edge opens: icon size and grid
// shape as pictures to pick from, and whether the names show. Saved when it closes; Home redraws behind it.
class HomeLayoutActivity final : public PanelActivity {
 public:
  HomeLayoutActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onExit() override;

 protected:
  int contentHeight() const override;
  void buildContent(Screen& screen, int x, int y, int width) override;
  void onChoice(freeink::ui::ActionId action, int value) override;
};
