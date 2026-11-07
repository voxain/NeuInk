#include "BatteryActivity.h"

#if NEUINK_BATTERY_HISTORY

#include <HalPowerManager.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <utility>
#include <vector>

#include "MappedInputManager.h"
#include "components/BatteryGraph.h"
#include "components/BigText.h"
#include "components/themes/vega/VegaHome.h"
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

namespace {
// The AppUsage slot of a Home entry.
uint8_t usageSlotOf(const uint8_t key) {
  switch (key) {
    case vegahome::KeyFiles:
      return AppUsage::kFiles;
    case vegahome::KeyLibrary:
      return AppUsage::kLibrary;
    case vegahome::KeyOpds:
      return AppUsage::kOpds;
    case vegahome::KeyStats:
      return AppUsage::kStats;
    case vegahome::KeySaved:
      return AppUsage::kSaved;
    default:
      return AppUsage::kSettings;
  }
}
}  // namespace

BatteryActivity::BatteryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Battery", renderer, mappedInput) {}

void BatteryActivity::onEnter() {
  Activity::onEnter();
  firstRow = 0;
  requestUpdate();
}

void BatteryActivity::loop() {
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }
  const auto swipe = mappedInput.wasSwipe();
  if ((swipe == MappedInputManager::SwipeDir::Up || mappedInput.wasReleased(MappedInputManager::Button::Down)) &&
      moreBelow) {
    firstRow += std::max(1, shownRows);
    requestUpdate();
  } else if ((swipe == MappedInputManager::SwipeDir::Down || mappedInput.wasReleased(MappedInputManager::Button::Up)) &&
             firstRow > 0) {
    firstRow = std::max(0, firstRow - std::max(1, shownRows));
    requestUpdate();
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

  int ey = y;
  for (const auto& estimate : {std::pair<StrId, int>{StrId::STR_BATTERY_SCREEN_ON, rates.awakeTenths},
                               std::pair<StrId, int>{StrId::STR_BATTERY_ASLEEP, rates.sleepTenths}}) {
    char value[24];
    batterygraph::runtimeText(estimate.second, percent, value, sizeof(value));
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

  // The graph on a gray card (components/BatteryGraph).
  renderer.fillRoundedRect(kSide, y, width - 2 * kSide, kCardHeight, 24, Color::LightGray);
  const int plotX = kSide + kCardPad + kAxisWidth;
  batterygraph::draw(renderer, Rect{plotX, y + kCardPad, width - kSide - kCardPad - plotX,
                                    kCardHeight - 2 * kCardPad - kAxisHeight},
                     levels, haveLevels, true);
  y += kCardHeight + 14;

  // Time in each app today: a gray row per app that was opened.
  y += 10;
  renderer.drawText(UI_10_FONT_ID, kSide, y, tr(STR_APPS_TIME), true, EpdFontFamily::BOLD);
  y += renderer.getLineHeight(UI_10_FONT_ID) + 8;
  bool any = false;
  // More apps than fit scroll a page at a time (a swipe or the up and down buttons): rows before firstRow are
  // skipped, and the rows stop at the button hints.
  constexpr int rowHeight = 40;
  const int listTop = y;
  const int listBottom = renderer.getScreenHeight() - UITheme::getInstance().getMetrics().buttonHintsHeight - 8;
  int rowIndex = 0;
  shownRows = 0;
  moreBelow = false;
  const auto row = [&](const StrId name, const uint8_t slot, const uint32_t week) {
    const int rate = AppUsage::drainTenthsPerHour(slot);
    // Everything that is not an app has no clock, only the rate.
    const bool other = slot == 0;
    if (other ? rate < 0 : week == 0) return;
    any = true;
    const int index = rowIndex++;
    if (index < firstRow) return;
    if (moreBelow || y + rowHeight > listBottom) {
      moreBelow = true;
      return;
    }
    ++shownRows;
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
  // The reader, Home and everything with an icon on Home, the most used first; then what ran outside all of them.
  struct Used {
    StrId name;
    uint8_t slot;
    uint32_t week;
  };
  std::vector<Used> used;
  used.reserve(AppUsage::kSlots);
  const auto add = [&used](const StrId name, const uint8_t slot) {
    used.push_back({name, slot, AppUsage::secondsWeek(slot)});
  };
  add(StrId::STR_CAT_READER, AppUsage::kReader);
  add(StrId::STR_HOME, AppUsage::kHome);
  for (const vegahome::EntryInfo& entry : vegahome::kEntries) add(entry.name, usageSlotOf(entry.key));
  for (const apps::Info& app : apps::kAll) add(app.name, app.id);
  std::stable_sort(used.begin(), used.end(), [](const Used& a, const Used& b) { return a.week > b.week; });
  for (const Used& entry : used) row(entry.name, entry.slot, entry.week);
  row(StrId::STR_APPS_OTHER, 0, 0);
  if (!any) renderer.drawText(SMALL_FONT_ID, kSide, y, tr(STR_NO_APP_TIME), true);
  if (firstRow > 0 || moreBelow) {
    // A thin bar at the right edge: which part of the list shows.
    const int trackH = listBottom - listTop;
    const int thumbY = listTop + trackH * firstRow / rowIndex;
    const int thumbH = std::max(16, trackH * shownRows / rowIndex);
    renderer.fillRoundedRect(width - 8, listTop, 4, trackH, 2, Color::LightGray);
    renderer.fillRoundedRect(width - 8, std::min(thumbY, listTop + trackH - thumbH), 4, thumbH, 2, Color::Black);
  }

  const auto labels = mappedInput.mapLabels(mappedInput.withBackArrow(tr(STR_BACK)), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}

#endif
