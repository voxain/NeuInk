#include "BatteryActivity.h"

#if NEUINK_BATTERY_HISTORY

#include <HalPowerManager.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <utility>

#include "MappedInputManager.h"
#include "components/BigText.h"
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
  const Rect header = TouchHeaderBackButton::drawPageHeader(renderer, mappedInput, tr(STR_BATTERY));

  const int width = renderer.getScreenWidth();
  int y = header.y + header.height + 24;

  // Now: the level, big (the page's main number), the cell voltage under it, and on the right how long the charge
  // lasts: with the screen on and asleep, from the drain in the last 24 hours.
  uint8_t levels[BatteryHistory::kBuckets];
  const bool haveLevels = BatteryHistory::read(levels);
  const BatteryHistory::Rates rates = haveLevels ? BatteryHistory::rates(levels) : BatteryHistory::Rates{};
  const unsigned percent = powerManager.getBatteryPercentage();

  char text[24];
  snprintf(text, sizeof(text), "%u%%", percent);
  const int bigHeight = drawBigText(renderer, UI_12_FONT_ID, text, kSide, y);
  const int volts = powerManager.getBatteryMillivolts();
  snprintf(text, sizeof(text), "%d.%02d V", volts / 1000, (volts % 1000) / 10);
  renderer.drawText(UI_10_FONT_ID, kSide, y + bigHeight, text, true);

  // "~6h 20m", or days from two days on.
  const auto runtime = [&](const int tenths, char* out, const size_t size) {
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
  };
  int ey = y;
  for (const auto& estimate : {std::pair<StrId, int>{StrId::STR_BATTERY_SCREEN_ON, rates.awakeTenths},
                               std::pair<StrId, int>{StrId::STR_BATTERY_ASLEEP, rates.sleepTenths}}) {
    char value[24];
    runtime(estimate.second, value, sizeof(value));
    const int right = width - kSide;
    renderer.drawText(SMALL_FONT_ID, right - renderer.getTextWidth(SMALL_FONT_ID, I18N.get(estimate.first)), ey,
                      I18N.get(estimate.first), true);
    ey += renderer.getLineHeight(SMALL_FONT_ID);
    renderer.drawText(UI_10_FONT_ID, right - renderer.getTextWidth(UI_10_FONT_ID, value, EpdFontFamily::BOLD), ey,
                      value, true, EpdFontFamily::BOLD);
    ey += renderer.getLineHeight(UI_10_FONT_ID) + 10;
  }
  y = std::max(y + bigHeight + renderer.getLineHeight(UI_10_FONT_ID) + 16, ey + 6);

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
  if (haveLevels) {
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
