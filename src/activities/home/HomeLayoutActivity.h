#pragma once

#include "components/PanelActivity.h"

// The Vega Home layout, a panel over Home that a swipe up from the bottom edge opens: icon size and shape as pictures
// to pick from, the grid's columns and rows, the names, the greeting, and the pencil in its corner that puts Home
// into edit mode (its result is then a MenuResult with action kEditHome). Saved when it closes; Home redraws behind it.
class HomeLayoutActivity final : public PanelActivity {
 public:
  static constexpr int kEditHome = 1;

  HomeLayoutActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onExit() override;

 protected:
  int sectionCount() const override;
  int sectionHeight(int section) const override;
  void buildSection(Screen& screen, int section, int x, int y, int width) override;
  void onChoice(freeink::ui::ActionId action, int value) override;
  int maxContentHeight() const override;
  const freeink::Icon* cornerIcon() const override;
  void onCorner() override;
};
