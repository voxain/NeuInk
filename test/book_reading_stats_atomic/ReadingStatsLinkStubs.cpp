#include <ReadingStatsUtils.h>

bool ReadingStatsDate::isValid() const { return year >= 2000 && month >= 1 && month <= 12 && day >= 1 && day <= 31; }

void ReadingStatsDate::clear() {
  year = 0;
  month = 0;
  day = 0;
}

void recordReadingSpanIntoBuckets(std::array<uint32_t, READING_TIME_BUCKET_COUNT>&,
                                  std::array<uint32_t, READING_DAY_OF_WEEK_COUNT>&, const ReadingStatsDateTime&,
                                  uint32_t) {}

// The time-left and finish-date estimates are not what this test covers.
bool ReadingStatsDateTime::isValid() const { return date.isValid(); }

void addSecondsToReadingStatsDateTime(ReadingStatsDateTime&, uint32_t) {}

uint16_t readingSpanDaysElapsed(const ReadingStatsDate&, const ReadingStatsDate&) { return 0; }
