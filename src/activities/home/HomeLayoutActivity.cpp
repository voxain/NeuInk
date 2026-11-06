#include "HomeLayoutActivity.h"

#include <I18n.h>

#include "CrossPointSettings.h"

namespace {
const std::vector<std::string> kIconSizes = {"32 px", "42 px", "48 px"};
const std::vector<std::string> kGrids = {"4 x 3", "4 x 4", "3 x 4", "3 x 3"};
}  // namespace

HomeLayoutActivity::HomeLayoutActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : RowListActivity(renderer, mappedInput, "HomeLayout", tr(STR_HOME_LAYOUT)) {}

void HomeLayoutActivity::choose(const char* title, const std::vector<std::string>& options, uint8_t& value) {
  std::vector<const char*> labels;
  for (const std::string& option : options) labels.push_back(option.c_str());
  popup.show(title, labels.data(), static_cast<int>(labels.size()), value, [this, &value](const int index) {
    value = static_cast<uint8_t>(index);
    SETTINGS.saveToFile();
    refresh();
  });
  requestUpdate();
}

void HomeLayoutActivity::buildRows(std::vector<Row>& rows) {
  rows.clear();
  const auto add = [&rows](const Row::Kind kind, const char* label, std::string value, std::function<void()> onTap,
                           const bool checked = false) {
    Row row;
    row.kind = kind;
    row.label = label;
    row.value = std::move(value);
    row.checked = checked;
    row.onTap = std::move(onTap);
    rows.push_back(std::move(row));
  };
  add(Row::Kind::Value, tr(STR_ICON_SIZE), kIconSizes[SETTINGS.homeIconSize],
      [this] { choose(tr(STR_ICON_SIZE), kIconSizes, SETTINGS.homeIconSize); });
  add(Row::Kind::Value, tr(STR_HOME_GRID), kGrids[SETTINGS.homeGrid],
      [this] { choose(tr(STR_HOME_GRID), kGrids, SETTINGS.homeGrid); });
  add(Row::Kind::Toggle, tr(STR_SHOW_NAMES), "",
      [this] {
        SETTINGS.homeShowNames = !SETTINGS.homeShowNames;
        SETTINGS.saveToFile();
        refresh();
      },
      SETTINGS.homeShowNames != 0);
  add(Row::Kind::Chevron, tr(STR_RESET_LAYOUT), "", [this] {
    SETTINGS.homeIconSize = 0;
    SETTINGS.homeGrid = 0;
    SETTINGS.homeShowNames = 1;
    SETTINGS.saveToFile();
    refresh();
  });
}
