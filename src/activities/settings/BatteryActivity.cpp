#include "BatteryActivity.h"

#if NEUINK_BATTERY_HISTORY

#include <HalPowerManager.h>
#include <I18n.h>

#include <cstdio>

#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "activities/apps/AppRegistry.h"
#include "activities/reader/ReadingStatsUtils.h"
#include "fontIds.h"
#include "util/AppUsage.h"

namespace {
constexpr int kSide = 16;
constexpr int kCardPad = 20;
constexpr int kCardHeight = 250;
constexpr int kAxisWidth = 44;  // room for the % labels
constexpr int kAxisHeight = 30;  // room for the time labels
}  // namespace

BatteryActivity::BatteryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Battery", renderer, mappedInput) {}

void BatteryActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

void BatteryActivity::loop() {
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
  }
}

void BatteryActivity::render(RenderLock&&) {
  renderer.clearScreen();
  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  const char* title = tr(STR_BATTERY);
  if (mappedInput.hasTouchHardware()) {
    freeink::ui::GfxRendererTarget target(renderer);
    TouchHeaderBackButton::draw(renderer, target, header, title, false);
  } else {
    GUI.drawHeader(renderer, header, title);
  }

  const int width = renderer.getScreenWidth();
  int y = header.y + header.height + 24;

  // Now: the level big, the cell voltage beside it.
  char text[24];
  snprintf(text, sizeof(text), "%u%%", static_cast<unsigned>(powerManager.getBatteryPercentage()));
  renderer.drawText(UI_12_FONT_ID, kSide, y, text, true, EpdFontFamily::BOLD);
  const int volts = powerManager.getBatteryMillivolts();
  snprintf(text, sizeof(text), "%d.%02d V", volts / 1000, (volts % 1000) / 10);
  renderer.drawText(UI_10_FONT_ID, width - kSide - renderer.getTextWidth(UI_10_FONT_ID, text),
                    y + renderer.getLineHeight(UI_12_FONT_ID) - renderer.getLineHeight(UI_10_FONT_ID) - 2, text, true);
  y += renderer.getLineHeight(UI_12_FONT_ID) + 16;

  renderer.drawText(UI_10_FONT_ID, kSide, y, tr(STR_BATTERY_LAST_24H), true, EpdFontFamily::BOLD);
  y += renderer.getLineHeight(UI_10_FONT_ID) + 10;

  // The graph on a gray card: gridlines at 0, 50 and 100 %, the level as a line, a gap where the reader slept.
  renderer.fillRoundedRect(kSide, y, width - 2 * kSide, kCardHeight, 24, Color::LightGray);
  const int plotX = kSide + kCardPad + kAxisWidth;
  const int plotY = y + kCardPad;
  const int plotW = width - kSide - kCardPad - plotX;
  const int plotH = kCardHeight - 2 * kCardPad - kAxisHeight;
  for (const int percent : {0, 50, 100}) {
    const int lineY = plotY + plotH * (100 - percent) / 100;
    renderer.drawLine(plotX, lineY, plotX + plotW, lineY, 2, false);
    snprintf(text, sizeof(text), "%d", percent);
    renderer.drawText(SMALL_FONT_ID, plotX - 8 - renderer.getTextWidth(SMALL_FONT_ID, text),
                      lineY - renderer.getLineHeight(SMALL_FONT_ID) / 2, text, true);
  }
  // The left half is the last 24 hours, the right half the next 24: an estimate that continues the level's trend.
  const int half = plotW / 2;
  const int middle = plotX + half;
  renderer.drawLine(middle, plotY, middle, plotY + plotH, 2, false);
  const int labelY = plotY + plotH + 8;
  renderer.drawText(SMALL_FONT_ID, plotX, labelY, "-24 h", true);
  renderer.drawText(SMALL_FONT_ID, middle - renderer.getTextWidth(SMALL_FONT_ID, "0") / 2, labelY, "0", true);
  renderer.drawText(SMALL_FONT_ID, plotX + plotW - renderer.getTextWidth(SMALL_FONT_ID, "+24 h"), labelY, "+24 h", true);

  const auto levelY = [&](const float level) { return plotY + static_cast<int>(plotH * (100.0f - level) / 100.0f); };
  uint8_t levels[BatteryHistory::kBuckets];
  if (BatteryHistory::read(levels)) {
    constexpr int last = BatteryHistory::kBuckets - 1;
    int previousX = 0;
    int previousY = 0;
    bool havePrevious = false;
    int lastIndex = -1;
    int fitFrom = 0;  // the trend starts after the latest recharge
    for (int i = 0; i <= last; ++i) {
      if (levels[i] == BatteryHistory::kNone) {
        havePrevious = false;
        continue;
      }
      if (lastIndex >= 0 && levels[i] > levels[lastIndex] + 1) fitFrom = i;
      const int px = plotX + half * i / last;
      const int py = levelY(levels[i]);
      if (havePrevious) renderer.drawLine(previousX, previousY, px, py, 3, true);
      previousX = px;
      previousY = py;
      havePrevious = true;
      lastIndex = i;
    }

    // Least-squares line through the points since the last recharge, continued from the current level.
    float count = 0, sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    for (int i = fitFrom; i <= last; ++i) {
      if (levels[i] == BatteryHistory::kNone) continue;
      count += 1;
      sumX += i;
      sumY += levels[i];
      sumXY += static_cast<float>(i) * levels[i];
      sumXX += static_cast<float>(i) * i;
    }
    const float denominator = count * sumXX - sumX * sumX;
    if (lastIndex >= 0 && count >= 3 && denominator > 0.0f) {
      const float slope = (count * sumXY - sumX * sumY) / denominator;  // percent per bucket
      const float from = levels[lastIndex];
      const int steps = BatteryHistory::kBuckets;
      int fromX = plotX + half * lastIndex / last;
      int fromY = levelY(from);
      for (int k = 1; k <= steps; ++k) {
        float level = from + slope * static_cast<float>(last + k - lastIndex);
        level = level < 0.0f ? 0.0f : (level > 100.0f ? 100.0f : level);
        const int toX = middle + half * k / steps;
        const int toY = levelY(level);
        if ((k / 12) % 2 == 0) renderer.drawLine(fromX, fromY, toX, toY, 3, true);  // dashed: an estimate
        fromX = toX;
        fromY = toY;
        if (level <= 0.0f || level >= 100.0f) break;
      }
    }
  }
  y += kCardHeight + 14;

  // What the gaps are.
  for (const auto& line : renderer.wrappedText(SMALL_FONT_ID, tr(STR_BATTERY_NOTE), width - 2 * kSide, 3)) {
    renderer.drawText(SMALL_FONT_ID, kSide, y, line.c_str(), true);
    y += renderer.getLineHeight(SMALL_FONT_ID);
  }

  // Time in each app today: a gray row per app that was opened.
  y += 10;
  renderer.drawText(UI_10_FONT_ID, kSide, y, tr(STR_APPS_TIME), true, EpdFontFamily::BOLD);
  y += renderer.getLineHeight(UI_10_FONT_ID) + 8;
  bool any = false;
  // The reader first, then the apps.
  const auto row = [&](const StrId name, const uint8_t slot) {
    const uint32_t week = AppUsage::secondsWeek(slot);
    const int rate = AppUsage::drainTenthsPerHour(slot);
    // Everything that is not an app has no clock, only the rate.
    const bool other = slot == 0;
    if (other ? rate < 0 : week == 0) return;
    any = true;
    constexpr int rowHeight = 40;
    renderer.fillRoundedRect(kSide, y, width - 2 * kSide, rowHeight, rowHeight / 2, Color::LightGray);
    const int textY = y + (rowHeight - renderer.getLineHeight(UI_10_FONT_ID)) / 2;
    renderer.drawText(UI_10_FONT_ID, kSide + 20, textY, I18N.get(name), true);
    // What it costs in charge, from the percent lost while it was in front: "4.2 %/h".
    if (rate >= 0) {
      char cost[16];
      snprintf(cost, sizeof(cost), "%d.%d %%/h", rate / 10, rate % 10);
      renderer.drawText(SMALL_FONT_ID, kSide + 20 + renderer.getTextWidth(UI_10_FONT_ID, I18N.get(name)) + 14,
                        y + (rowHeight - renderer.getLineHeight(SMALL_FONT_ID)) / 2, cost, true);
    }
    if (other) {
      y += rowHeight + 6;
      return;
    }
    // "today · week"
    char today[16];
    char thisWeek[16];
    char duration[40];
    formatCompactReadingDuration(AppUsage::secondsToday(slot), today, sizeof(today));
    formatCompactReadingDuration(week, thisWeek, sizeof(thisWeek));
    snprintf(duration, sizeof(duration), "%s  \xC2\xB7  %s", today, thisWeek);
    renderer.drawText(UI_10_FONT_ID, width - kSide - 20 - renderer.getTextWidth(UI_10_FONT_ID, duration, EpdFontFamily::BOLD),
                      textY, duration, true, EpdFontFamily::BOLD);
    y += rowHeight + 6;
  };
  row(StrId::STR_CAT_READER, AppUsage::kReader);
  for (const apps::Info& app : apps::kAll) row(app.name, app.id);
  row(StrId::STR_APPS_OTHER, 0);
  if (!any) renderer.drawText(SMALL_FONT_ID, kSide, y, tr(STR_NO_APP_TIME), true);

  const auto labels = mappedInput.mapLabels(mappedInput.withBackArrow(tr(STR_BACK)), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}

#endif
