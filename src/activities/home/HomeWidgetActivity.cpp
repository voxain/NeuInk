#include "HomeWidgetActivity.h"

#include <I18n.h>

#include <algorithm>
#include <cstdio>

#include "CrossPointSettings.h"
#include "components/UiAppHelpers.h"
#include "components/themes/vega/VegaHome.h"
#include "fontIds.h"

namespace {
constexpr freeink::ui::ActionId ACTION_SIZE = PanelActivity::ACTION_FIRST_CONTROL;
constexpr freeink::ui::ActionId ACTION_TOGGLE = PanelActivity::ACTION_FIRST_CONTROL + 1;
constexpr freeink::ui::ActionId ACTION_NUMBER = PanelActivity::ACTION_FIRST_CONTROL + 2;
constexpr freeink::ui::ActionId ACTION_ADD = PanelActivity::ACTION_FIRST_CONTROL + 3;
constexpr int kTitleHeight = 48;

constexpr const char* kSizeLabel[vegahome::kSizeCount] = {"1x1", "2x1", "2x2"};

// A size's shape as cells: one, two side by side, two by two.
void sizePreview(const GfxRenderer& renderer, const int cx, const int top, const bool on, const int size,
                 const bool available) {
  constexpr int cell = 22;
  constexpr int gap = 4;
  const int columns = size == vegahome::Small ? 1 : 2;
  const int rows = size == vegahome::Large ? 2 : 1;
  const int left = cx - (columns * cell + (columns - 1) * gap) / 2;
  const int first = top + (panel::kPreviewHeight - (rows * cell + (rows - 1) * gap)) / 2;
  const Color color = !available ? Color::LightGray : (on ? Color::White : Color::Black);
  for (int row = 0; row < rows; ++row) {
    for (int column = 0; column < columns; ++column) {
      renderer.fillRoundedRect(left + column * (cell + gap), first + row * (cell + gap), cell, cell, 6, color);
    }
  }
}
}  // namespace

HomeWidgetActivity::HomeWidgetActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const Target& target)
    : PanelActivity(renderer, mappedInput, "HomeWidget"),
      target(target),
      widget(target.kind == Target::Widget ? target.widget : -1) {}

freeink::ui::SheetEdge HomeWidgetActivity::anchor() const {
  return target.atTop ? freeink::ui::SheetEdge::Top : freeink::ui::SheetEdge::Bottom;
}

HomeWidgetActivity::Item HomeWidgetActivity::item(const int section) const {
  if (section == 0) return {Title, 0};
  if (target.kind == Target::Entry) return {section == 1 ? ShowEntry : Note, 0};
  if (widget < 0) return {AddRow, static_cast<uint8_t>(section - 1)};
  const vegahome::Info& info = vegahome::info(SETTINGS.homeWidgets[widget].type);
  int next = 1;
  const bool sizes = (info.sizes & (info.sizes - 1)) != 0;  // more than one
  if (sizes && section == next++) return {Sizes, 0};
  if (section < next + info.optionCount) return {Toggle, static_cast<uint8_t>(section - next)};
  return {Number, 0};
}

int HomeWidgetActivity::sectionCount() const {
  if (target.kind == Target::Entry) return 3;
  if (widget < 0) return 1 + vegahome::kTypeCount;
  const vegahome::Info& info = vegahome::info(SETTINGS.homeWidgets[widget].type);
  return 1 + ((info.sizes & (info.sizes - 1)) != 0 ? 1 : 0) + info.optionCount +
         (info.value.format != vegahome::Value::None ? 1 : 0);
}

int HomeWidgetActivity::sectionHeight(const int section) const {
  switch (item(section).part) {
    case Title:
      return kTitleHeight;
    case Sizes:
      return panel::kLabelHeight + panel::kTileHeight + panel::kRowGap;
    case Note:
      return 2 * renderer.getLineHeight(SMALL_FONT_ID) + 8;
    default:
      return panel::kControlHeight + panel::kRowGap / 2;
  }
}

const freeink::Icon* HomeWidgetActivity::cornerIcon() const { return widget >= 0 ? &icon_trash_24 : nullptr; }

void HomeWidgetActivity::onCorner() {
  if (widget < 0 || widget >= SETTINGS.homeWidgetCount) return;
  auto* widgets = SETTINGS.homeWidgets;
  std::copy(widgets + widget + 1, widgets + SETTINGS.homeWidgetCount, widgets + widget);
  --SETTINGS.homeWidgetCount;
  finish();
}

bool HomeWidgetActivity::sizeFits(const uint8_t size) const {
  vegahome::Widget widgets[CrossPointSettings::HOME_WIDGET_MAX];
  std::copy_n(SETTINGS.homeWidgets, SETTINGS.homeWidgetCount, widgets);
  widgets[widget].size = size;
  return vegahome::fits(widgets, SETTINGS.homeWidgetCount);
}

uint8_t HomeWidgetActivity::sizeForNew(const uint8_t type) const {
  if (SETTINGS.homeWidgetCount >= CrossPointSettings::HOME_WIDGET_MAX) return vegahome::kSizeCount;
  const vegahome::Info& info = vegahome::info(type);
  vegahome::Widget widgets[CrossPointSettings::HOME_WIDGET_MAX];
  std::copy_n(SETTINGS.homeWidgets, SETTINGS.homeWidgetCount, widgets);
  // Its own size first, then the smaller ones.
  for (const uint8_t size : {static_cast<uint8_t>(info.defaultSize), static_cast<uint8_t>(vegahome::Wide),
                             static_cast<uint8_t>(vegahome::Small)}) {
    if (!vegahome::hasSize(type, size)) continue;
    widgets[SETTINGS.homeWidgetCount] = vegahome::Widget{type, size, info.defaultFlags, info.value.initial};
    if (vegahome::fits(widgets, SETTINGS.homeWidgetCount + 1)) return size;
  }
  return vegahome::kSizeCount;
}

void HomeWidgetActivity::buildSection(Screen& screen, const int section, const int x, const int y, const int width) {
  auto& frame = screen.frame();
  const Item it = item(section);
  const int lineH10 = renderer.getLineHeight(UI_10_FONT_ID);
  switch (it.part) {
    case Title: {
      if (target.kind == Target::Entry) {
        if (const freeink::Icon* icon = homeIcon(target.icon, 24)) drawLucideIcon(renderer, *icon, x, y + 6);
        renderer.drawText(UI_12_FONT_ID, x + 34, y + 4, target.label, true, EpdFontFamily::BOLD);
        break;
      }
      if (widget < 0) {
        renderer.drawText(UI_12_FONT_ID, x, y + 4, tr(STR_ADD_WIDGET), true, EpdFontFamily::BOLD);
        break;
      }
      const vegahome::Info& info = vegahome::info(SETTINGS.homeWidgets[widget].type);
      drawLucideIcon(renderer, *info.icon, x, y + 6);
      renderer.drawText(UI_12_FONT_ID, x + 34, y + 4, I18N.get(info.name), true, EpdFontFamily::BOLD);
      break;
    }
    case Sizes: {
      const uint8_t type = SETTINGS.homeWidgets[widget].type;
      uint8_t shown[vegahome::kSizeCount];
      int count = 0;
      int selected = -1;
      for (uint8_t size = 0; size < vegahome::kSizeCount; ++size) {
        if (!vegahome::hasSize(type, size)) continue;
        if (size == SETTINGS.homeWidgets[widget].size) selected = count;
        shown[count++] = size;
      }
      panel::heading(renderer, x, y, tr(STR_WIDGET_SIZE), UI_10_FONT_ID);
      panel::tiles(
          renderer, frame, x, y + panel::kLabelHeight, width, count, selected, ACTION_SIZE, UI_10_FONT_ID,
          [&](const int cx, const int top, const bool on, const int i) {
            sizePreview(renderer, cx, top, on, shown[i], on || sizeFits(shown[i]));
          },
          [&](const int i) { return kSizeLabel[shown[i]]; });
      break;
    }
    case Toggle: {
      const vegahome::Widget& w = SETTINGS.homeWidgets[widget];
      const vegahome::Option& option = vegahome::info(w.type).options[it.index];
      panel::toggleRow(renderer, frame, x, y, width, I18N.get(option.label), (w.flags & option.flag) != 0,
                       ACTION_TOGGLE, UI_10_FONT_ID, it.index);
      break;
    }
    case Number: {
      const vegahome::Widget& w = SETTINGS.homeWidgets[widget];
      const vegahome::Value& option = vegahome::info(w.type).value;
      char value[16];
      vegahome::valueText(option, w.value, value, sizeof(value));
      panel::stepperRow(renderer, frame, x, y, width, I18N.get(option.label), value, ACTION_NUMBER, UI_10_FONT_ID);
      break;
    }
    case ShowEntry:
      panel::toggleRow(renderer, frame, x, y, width, tr(STR_SHOW_ON_HOME), true, ACTION_TOGGLE, UI_10_FONT_ID);
      break;
    case Note: {
      int lineY = y;
      for (const auto& line : renderer.wrappedText(SMALL_FONT_ID, tr(STR_HIDDEN_APP_NOTE), width - 8, 2)) {
        renderer.drawText(SMALL_FONT_ID, x + 4, lineY, line.c_str(), true);
        lineY += renderer.getLineHeight(SMALL_FONT_ID);
      }
      break;
    }
    case AddRow: {
      const uint8_t type = it.index;
      const vegahome::Info& info = vegahome::info(type);
      const uint8_t size = sizeForNew(type);
      renderer.fillRoundedRect(x, y, width, panel::kControlHeight, panel::kControlHeight / 2, Color::White);
      drawLucideIcon(renderer, *info.icon, x + 18, y + (panel::kControlHeight - 24) / 2);
      const int textY = y + (panel::kControlHeight - lineH10) / 2;
      renderer.drawText(UI_10_FONT_ID, x + 56, textY, I18N.get(info.name), true);
      const char* note = size < vegahome::kSizeCount ? kSizeLabel[size] : tr(STR_WIDGET_NO_ROOM);
      renderer.drawText(UI_10_FONT_ID, x + width - 22 - renderer.getTextWidth(UI_10_FONT_ID, note), textY, note, true);
      if (size < vegahome::kSizeCount) {
        frame.hit(freeink::ui::Rect{static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(width),
                                    panel::kControlHeight},
                  ACTION_ADD, static_cast<int16_t>(type), freeink::ui::InputTouch);
      }
      break;
    }
  }
}

void HomeWidgetActivity::onChoice(const freeink::ui::ActionId action, const int value) {
  if (action == ACTION_ADD) {
    const uint8_t type = static_cast<uint8_t>(value);
    const uint8_t size = sizeForNew(type);
    if (size >= vegahome::kSizeCount) return;
    const vegahome::Info& info = vegahome::info(type);
    SETTINGS.homeWidgets[SETTINGS.homeWidgetCount++] = vegahome::Widget{type, size, info.defaultFlags, info.value.initial};
    finish();
    return;
  }
  if (target.kind == Target::Entry) {
    // Off: the icon leaves Home (Settings > Apps brings it back).
    vegahome::setEntryHidden(target.key, true);
    finish();
    return;
  }
  if (widget < 0 || widget >= SETTINGS.homeWidgetCount) return;
  vegahome::Widget& w = SETTINGS.homeWidgets[widget];
  if (action == ACTION_SIZE) {
    // The tiles' index among the sizes the widget comes in.
    int index = 0;
    for (uint8_t size = 0; size < vegahome::kSizeCount; ++size) {
      if (!vegahome::hasSize(w.type, size)) continue;
      if (index++ == value && sizeFits(size)) w.size = size;
    }
  } else if (action == ACTION_TOGGLE && value >= 0 && value < vegahome::info(w.type).optionCount) {
    w.flags ^= vegahome::info(w.type).options[value].flag;
  } else if (action == ACTION_NUMBER) {
    const vegahome::Value& option = vegahome::info(w.type).value;
    w.value = static_cast<uint8_t>(std::clamp(w.value + value * option.step, 0, static_cast<int>(option.max)));
  }
  requestUpdate();
}
