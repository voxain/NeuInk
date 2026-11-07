#pragma once

#include <cstddef>
#include <cstdint>

#include "components/themes/BaseTheme.h"
#include "util/BatteryHistory.h"

class GfxRenderer;

#if NEUINK_BATTERY_HISTORY
// The battery graph of Settings > Battery and Home's battery widget: the last 24 hours of charge on the left half
// of `plot` (one line through the day, the time the reader was awake shaded under it), and on the right half the
// next 24 hours as a dashed estimate that continues the trend since the last recharge. White gridlines at 0, 50
// and 100 %; `axes` adds their labels left of the plot and the time labels under it (outside `plot`).
namespace batterygraph {

void draw(const GfxRenderer& renderer, const Rect& plot, const uint8_t (&levels)[BatteryHistory::kBuckets],
          bool haveLevels, bool axes);

// How long `percent` lasts at a drain of `tenths` percent per hour: "~6h 20m", days from two days on, "-" unknown.
void runtimeText(int tenths, unsigned percent, char* out, size_t size);

}  // namespace batterygraph
#endif
