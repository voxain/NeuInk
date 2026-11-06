#include "BatteryActivity.h"

#if NEUINK_BATTERY_HISTORY

#include <HalPowerManager.h>
#include <I18n.h>

#include <cstdio>

#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int kSide = 16;
constexpr int kCardPad = 20;
constexpr int kCardHeight = 300;
constexpr int kAxisWidth = 44;  // room for the % labels
constexpr int kAxisHeight = 30;  // room for the time labels
}  // namespace

BatteryActivity::BatteryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Battery", renderer, mappedInput) {}

void BatteryActivity::onEnter() {
  Activity::onEnter();
  BatteryHistory::sample();  // the graph ends with now
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
  const int labelY = plotY + plotH + 8;
  renderer.drawText(SMALL_FONT_ID, plotX, labelY, "-24 h", true);
  renderer.drawText(SMALL_FONT_ID, plotX + plotW / 2 - renderer.getTextWidth(SMALL_FONT_ID, "-12 h") / 2, labelY, "-12 h",
                    true);
  renderer.drawText(SMALL_FONT_ID, plotX + plotW - renderer.getTextWidth(SMALL_FONT_ID, "0"), labelY, "0", true);

  uint8_t levels[BatteryHistory::kBuckets];
  if (BatteryHistory::read(levels)) {
    constexpr int last = BatteryHistory::kBuckets - 1;
    int previousX = 0;
    int previousY = 0;
    bool havePrevious = false;
    for (int i = 0; i <= last; ++i) {
      if (levels[i] == BatteryHistory::kNone) {
        havePrevious = false;
        continue;
      }
      const int px = plotX + plotW * i / last;
      const int py = plotY + plotH * (100 - levels[i]) / 100;
      if (havePrevious) renderer.drawLine(previousX, previousY, px, py, 3, true);
      renderer.fillRoundedRect(px - 3, py - 3, 7, 7, 3, Color::Black);
      previousX = px;
      previousY = py;
      havePrevious = true;
    }
  }
  y += kCardHeight + 14;

  // What the gaps are.
  for (const auto& line : renderer.wrappedText(SMALL_FONT_ID, tr(STR_BATTERY_NOTE), width - 2 * kSide, 3)) {
    renderer.drawText(SMALL_FONT_ID, kSide, y, line.c_str(), true);
    y += renderer.getLineHeight(SMALL_FONT_ID);
  }

  const auto labels = mappedInput.mapLabels(mappedInput.withBackArrow(tr(STR_BACK)), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}

#endif
