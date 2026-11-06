#pragma once

#include <vector>

#include "RecentBooksStore.h"
#include "activities/settings/RowListActivity.h"

// Reading Stats starts here: "All Books" on top, then every recent book with its progress, so the stats to see
// are chosen first. A pick opens that page; back from it returns to this list.
class StatsPickerActivity final : public RowListActivity {
 public:
  StatsPickerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

 protected:
  void buildRows(std::vector<Row>& rows) override;

 private:
  // book: nullptr for all books.
  void openStats(const RecentBook* book);
};
