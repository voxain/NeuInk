#pragma once

#include "components/PanelActivity.h"

// The panels of the Vega Home edit mode, over Home: the widgets to add (widget = -1), or one widget's options: its
// size, the data it shows, and the bin in the corner that removes it. Changes go to SETTINGS at once; Home saves them
// when edit mode ends.
class HomeWidgetActivity final : public PanelActivity {
 public:
  HomeWidgetActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, int widget);

 protected:
  int sectionCount() const override;
  int sectionHeight(int section) const override;
  void buildSection(Screen& screen, int section, int x, int y, int width) override;
  void onChoice(freeink::ui::ActionId action, int value) override;
  const freeink::Icon* cornerIcon() const override;
  void onCorner() override;

 private:
  enum Part : uint8_t { Title, Sizes, Toggle, Minutes, AddRow };
  struct Item {
    Part part;
    uint8_t index;  // the option (Toggle) or the widget type (AddRow)
  };
  Item item(int section) const;
  bool sizeFits(uint8_t size) const;
  // The first size of a new widget of `type` that still fits, or kSizeCount.
  uint8_t sizeForNew(uint8_t type) const;

  int widget;
};
