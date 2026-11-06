#pragma once

#include "activities/settings/RowListActivity.h"

// The Vega Home grid's layout, opened by swiping up from the bottom edge of Home: icon size, grid shape and
// whether the names show. A change shows on Home as soon as this page closes.
class HomeLayoutActivity final : public RowListActivity {
 public:
  HomeLayoutActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

 protected:
  void buildRows(std::vector<Row>& rows) override;

 private:
  void choose(const char* title, const std::vector<std::string>& options, uint8_t& value);
};
