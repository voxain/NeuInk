#include "StatsPickerActivity.h"

#include <I18n.h>

#include "RecentBookProgress.h"
#include "RecentBookStats.h"
#include "activities/reader/BookStatsActivity.h"
#include "activities/reader/BookStatsTracking.h"
#include "activities/reader/GlobalReadingStats.h"

StatsPickerActivity::StatsPickerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : RowListActivity(renderer, mappedInput, "StatsPicker", tr(STR_READING_STATS)) {}

void StatsPickerActivity::buildRows(std::vector<Row>& rows) {
  rows.clear();
  Row all;
  all.label = tr(STR_STATS_ALL_TIME);
  all.kind = Row::Kind::Chevron;
  all.onTap = [this] { openStats(nullptr); };
  rows.push_back(std::move(all));

  // Copies: the list is rebuilt, and a book may be added or dropped while a stats page is open.
  for (const RecentBook& book : RECENT_BOOKS.getBooks()) {
    Row row;
    row.label = book.title;
    row.subtitle = book.author;
    row.value = RecentBookProgress::formatPercent(RecentBookStats::loadProgress(book));
    row.onTap = [this, book] { openStats(&book); };
    rows.push_back(std::move(row));
  }
}

void StatsPickerActivity::openStats(const RecentBook* book) {
  std::string title = tr(STR_READING_STATS);
  std::string cachePath;
  BookReadingStats stats;
  float progress = -1.0f;
  if (book != nullptr) {
    const std::string folder = RecentBookStats::cachePath(*book);
    // A book whose tracking is off has no page of its own: its stats are the all-books ones.
    if (!folder.empty() && BookStatsTracking::isBookEnabled(folder)) {
      cachePath = folder;
      title = book->title;
      stats = RecentBookStats::loadStats(*book);
      progress = RecentBookStats::loadProgress(*book);
    }
  }
  const GlobalReadingStats device = GlobalReadingStats::load();
  std::unique_ptr<BookStatsActivity> activity;
  if (GlobalReadingStats::hasSyncedStats()) {
    activity = std::make_unique<BookStatsActivity>(renderer, mappedInput, title, cachePath, stats, progress, false, 0,
                                                   device, GlobalReadingStats::loadAggregated(device));
  } else {
    activity = std::make_unique<BookStatsActivity>(renderer, mappedInput, title, cachePath, stats, progress, false, 0,
                                                   device);
  }
  if (book != nullptr && !cachePath.empty()) activity->setRatedBook(book->path, book->author);
  startActivityForResult(std::move(activity), [this](const ActivityResult&) { refresh(); });
}
