#pragma once

#include <string>

#include "../reader/BookReadingStats.h"

struct RecentBook;

// What the Home screen and the stats picker need to know about a recent book's reading statistics.
namespace RecentBookStats {
// The book's cache folder, where its stats and progress live; empty for a type without one.
std::string cachePath(const RecentBook& book);
// The stats the book's owner lets us show: all of them, or only the reading pace while tracking is off.
BookReadingStats visibleStats(const RecentBook& book, BookReadingStats stats);
BookReadingStats loadStats(const RecentBook& book);
// 0-100, or negative when unknown.
float loadProgress(const RecentBook& book);
}  // namespace RecentBookStats
