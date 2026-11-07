#pragma once

#include "components/PanelActivity.h"
#include "components/themes/BaseTheme.h"

// The panels of the Vega Home edit mode, over Home: the widgets to add, one widget's options (its size, the data it
// shows, and the bin in the corner that removes it), or an icon's (whether it shows on Home). Changes go to SETTINGS
// at once and show behind the panel (setBackdrop); Home saves them when edit mode ends.
class HomeWidgetActivity final : public PanelActivity {
 public:
  struct Target {
    enum Kind : uint8_t { Add, Widget, Entry } kind = Add;
    int widget = -1;           // Widget
    uint8_t key = 0;           // Entry: its vegahome entry key
    const char* label = "";    // Entry
    UIIcon icon = UIIcon::Apps;  // Entry
    bool atTop = false;        // hang from the top, to keep what is edited (in the lower half) in view
  };

  HomeWidgetActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const Target& target);

 protected:
  int sectionCount() const override;
  int sectionHeight(int section) const override;
  void buildSection(Screen& screen, int section, int x, int y, int width) override;
  void onChoice(freeink::ui::ActionId action, int value) override;
  const freeink::Icon* cornerIcon() const override;
  void onCorner() override;
  freeink::ui::SheetEdge anchor() const override;

 private:
  enum Part : uint8_t { Title, Sizes, Toggle, Minutes, AddRow, ShowEntry, Note };
  struct Item {
    Part part;
    uint8_t index;  // the option (Toggle) or the widget type (AddRow)
  };
  Item item(int section) const;
  bool sizeFits(uint8_t size) const;
  // The first size of a new widget of `type` that still fits, or kSizeCount.
  uint8_t sizeForNew(uint8_t type) const;

  Target target;
  int widget;  // target.widget for a widget's options, -1 otherwise
};
