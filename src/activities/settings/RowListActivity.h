#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "activities/Activity.h"
#include "components/OptionPopup.h"
#include "components/UiAppHost.h"
#include "util/ButtonNavigator.h"

// A settings-style page made of the shared FreeInkUI list: section headers, rows with a value or a chevron,
// switches, and an optional second line under a row. Subclasses say what the rows are (buildRows) and what a
// tap does; the page rebuilds its rows after every tap. Used by the pages that need no layout of their own
// (Dictionaries, About, the Apps options).
class RowListActivity : public Activity {
 public:
  struct Row {
    enum class Kind : uint8_t { Header, Value, Toggle, Chevron };
    Kind kind = Kind::Value;
    std::string label;
    std::string value;
    std::string subtitle;  // a smaller second line under the label
    bool checked = false;  // Toggle
    int8_t icon = -1;      // a UIIcon before the label, -1 for none
    std::function<void()> onTap;  // empty: an information row
  };

  RowListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string name, std::string title);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

  // A row opens another page: when it comes back, the rows are rebuilt.
  void openChild(std::unique_ptr<Activity> child) {
    startActivityForResult(std::move(child), [this](const ActivityResult&) { refresh(); });
  }

 protected:
  virtual void buildRows(std::vector<Row>& rows) = 0;
  // Rebuild the rows and redraw (after state changed outside a tap, e.g. a popup's choice).
  void refresh();

  OptionPopup popup;

 private:
  using UiHost = UiAppHost<24, 4>;
  using UiApp = UiHost::App;

  static void listScreen(UiApp::ScreenType& screen, void* user);
  static void onRowEvent(const freeink::ui::ActionEvent& event, void* user);
  void buildListScreen(UiApp::ScreenType& screen);
  void activate(int index);
  void select(int index);
  bool selectable(int index) const;

  std::string title;
  std::vector<Row> rows;
  UiHost ui;
  ButtonNavigator buttonNavigator;
  int selectedIndex = 0;
  int visibleRows = 1;
  int topIndex = 0;
  freeink::ui::ListNav listNav;
};

// A page whose rows come from a function: for pages with no state of their own beyond what the function reads.
class LambdaRowList final : public RowListActivity {
 public:
  using Builder = std::function<void(std::vector<Row>&, RowListActivity&)>;
  LambdaRowList(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string name, std::string title,
                Builder builder)
      : RowListActivity(renderer, mappedInput, std::move(name), std::move(title)), builder(std::move(builder)) {}

 protected:
  void buildRows(std::vector<Row>& rows) override { builder(rows, *this); }

 private:
  Builder builder;
};
