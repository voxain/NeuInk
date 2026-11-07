#include "RecentBookStats.h"

#include <Epub.h>
#include <FsHelpers.h>

#include <functional>

#include "../reader/BookStatsTracking.h"
#include "RecentBookProgress.h"
#include "RecentBooksStore.h"

namespace RecentBookStats {

std::string cachePath(const RecentBook& book) {
  if (FsHelpers::hasEpubExtension(book.path)) {
    return Epub::resolveCachePathForFilePath(book.path, "/.crosspoint");
  }
  if (FsHelpers::hasXtcExtension(book.path)) {
    return "/.crosspoint/xtc_" + std::to_string(std::hash<std::string>{}(book.path));
  }
  if (FsHelpers::hasTxtExtension(book.path) || FsHelpers::hasMarkdownExtension(book.path)) {
    return "/.crosspoint/txt_" + std::to_string(std::hash<std::string>{}(book.path));
  }
  return "";
}

BookReadingStats visibleStats(const RecentBook& book, BookReadingStats stats) {
  const std::string folder = cachePath(book);
  if (!BookStatsTracking::isEnabled(folder)) {
    BookReadingStats paceOnly;
    paceOnly.avgSecondsPerForwardPage = stats.avgSecondsPerForwardPage;
    paceOnly.paceSampleCount = stats.paceSampleCount;
    paceOnly.estimatedTimeLeftSeconds = stats.estimatedTimeLeftSeconds;
    return paceOnly;
  }
  return stats;
}

BookReadingStats loadStats(const RecentBook& book) {
  if (!FsHelpers::hasEpubExtension(book.path) && !FsHelpers::hasXtcExtension(book.path)) {
    return BookReadingStats{};
  }

  return visibleStats(book, BookReadingStats::load(cachePath(book)));
}

float loadProgress(const RecentBook& book) {
  return FsHelpers::hasEpubExtension(book.path) ? RecentBookProgress::loadCachedEpubPercent(book)
                                                : RecentBookProgress::loadPercent(book);
}

}  // namespace RecentBookStats
