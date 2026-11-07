#include "BatteryGraph.h"

#if NEUINK_BATTERY_HISTORY

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>

#include "activities/reader/ReadingStatsUtils.h"
#include "fontIds.h"

namespace batterygraph {

void draw(const GfxRenderer& renderer, const Rect& plot, const uint8_t (&levels)[BatteryHistory::kBuckets],
          const bool haveLevels, const bool axes) {
  const int plotX = plot.x;
  const int plotY = plot.y;
  const int plotW = plot.width;
  const int plotH = plot.height;
  const int half = plotW / 2;
  const auto levelY = [&](const float level) { return plotY + static_cast<int>(plotH * (100.0f - level) / 100.0f); };
  if (haveLevels) {
    constexpr int last = BatteryHistory::kBuckets - 1;
    for (int i = 1; i <= last; ++i) {
      if (levels[i - 1] == BatteryHistory::kNone || levels[i] == BatteryHistory::kNone) continue;
      const int x0 = plotX + half * (i - 1) / last;
      const int x1 = plotX + half * i / last;
      const int y0 = levelY(levels[i - 1]);
      const int y1 = levelY(levels[i]);
      for (int x = x0; x < x1; ++x) {
        const int top = y0 + (y1 - y0) * (x - x0) / std::max(1, x1 - x0);
        renderer.fillRectDither(x, top, 1, plotY + plotH - top, Color::DarkGray);
      }
    }
  }
  for (const int percent : {0, 50, 100}) {
    const int lineY = plotY + plotH * (100 - percent) / 100;
    renderer.drawLine(plotX, lineY, plotX + plotW, lineY, 2, false);
    if (!axes) continue;
    char text[8];
    snprintf(text, sizeof(text), "%d", percent);
    renderer.drawText(SMALL_FONT_ID, plotX - 8 - renderer.getTextWidth(SMALL_FONT_ID, text),
                      lineY - renderer.getLineHeight(SMALL_FONT_ID) / 2, text, true);
  }
  // The left half is the last 24 hours, the right half the next 24: an estimate that continues the level's trend.
  const int middle = plotX + half;
  renderer.drawLine(middle, plotY, middle, plotY + plotH, 2, false);
  if (axes) {
    const int labelY = plotY + plotH + 8;
    renderer.drawText(SMALL_FONT_ID, plotX, labelY, "-24 h", true);
    renderer.drawText(SMALL_FONT_ID, middle - renderer.getTextWidth(SMALL_FONT_ID, "0") / 2, labelY, "0", true);
    renderer.drawText(SMALL_FONT_ID, plotX + plotW - renderer.getTextWidth(SMALL_FONT_ID, "+24 h"), labelY, "+24 h",
                      true);
  }

  if (haveLevels) {
    constexpr int last = BatteryHistory::kBuckets - 1;
    int previousX = 0;
    int previousY = 0;
    bool havePrevious = false;
    int lastIndex = -1;
    for (int i = 0; i <= last; ++i) {
      if (levels[i] == BatteryHistory::kNone) continue;  // asleep: the line joins the levels either side
      const int px = plotX + half * i / last;
      const int py = levelY(levels[i]);
      if (havePrevious) renderer.drawLine(previousX, previousY, px, py, 3, true);
      previousX = px;
      previousY = py;
      havePrevious = true;
      lastIndex = i;
    }

    // The estimate: from the current level down at the drain BatteryHistory::rates measures (the runtimes' own
    // figure, rises left out), so it never climbs.
    const BatteryHistory::Rates rates = BatteryHistory::rates(levels);
    if (lastIndex >= 0 && rates.overallTenths > 0) {
      const float perBucket = rates.overallTenths / 10.0f * BatteryHistory::kBucketMinutes / 60.0f;
      const float from = levels[lastIndex];
      const int steps = BatteryHistory::kBuckets;
      int fromX = plotX + half * lastIndex / last;
      int fromY = levelY(from);
      for (int k = 1; k <= steps; ++k) {
        const float level = std::max(0.0f, from - perBucket * static_cast<float>(last + k - lastIndex));
        const int toX = middle + half * k / steps;
        const int toY = levelY(level);
        if ((k / 12) % 2 == 0) renderer.drawLine(fromX, fromY, toX, toY, 3, true);  // dashed: an estimate
        fromX = toX;
        fromY = toY;
        if (level <= 0.0f) break;
      }
    }
  }
}

void runtimeText(const int tenths, const unsigned percent, char* out, const size_t size) {
  if (tenths <= 0 || percent == 0) {
    snprintf(out, size, "-");
    return;
  }
  const uint32_t seconds = static_cast<uint32_t>(static_cast<uint64_t>(percent) * 10 * 3600 / tenths);
  if (seconds >= 48 * 3600) {
    snprintf(out, size, "~%u %s", static_cast<unsigned>(seconds / 86400), tr(STR_BATTERY_DAYS));
  } else {
    char duration[16];
    formatCompactReadingDuration(seconds, duration, sizeof(duration));
    snprintf(out, size, "~%s", duration);
  }
}

}  // namespace batterygraph
#endif
