#include "HomeLayoutActivity.h"

#include <I18n.h>

#include <algorithm>

#include "CrossPointSettings.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace {
constexpr freeink::ui::ActionId ACTION_ICON = PanelActivity::ACTION_FIRST_CONTROL;
constexpr freeink::ui::ActionId ACTION_GRID = PanelActivity::ACTION_FIRST_CONTROL + 1;
constexpr freeink::ui::ActionId ACTION_NAMES = PanelActivity::ACTION_FIRST_CONTROL + 2;

// The options in the order shown, as the values stored in homeIconSize (0 = 32 px, the default) and homeGrid.
constexpr int kIconPx[] = {24, 32, 42, 48};
constexpr uint8_t kIconValue[] = {3, 0, 1, 2};
constexpr const char* kIconLabel[] = {"24", "32", "42", "48"};
constexpr int kGridColumns[] = {4, 4, 3, 3};
constexpr int kGridRows[] = {3, 4, 4, 3};
constexpr const char* kGridLabel[] = {"4x3", "4x4", "3x4", "3x3"};

int iconIndex() {
  for (int i = 0; i < 4; ++i) {
    if (kIconValue[i] == SETTINGS.homeIconSize) return i;
  }
  return 1;
}
}  // namespace

HomeLayoutActivity::HomeLayoutActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : PanelActivity(renderer, mappedInput, "HomeLayout") {}

void HomeLayoutActivity::onExit() {
  SETTINGS.saveToFile();
  PanelActivity::onExit();
}

int HomeLayoutActivity::contentHeight() const {
  return 2 * (panel::kLabelHeight + panel::kTileHeight + panel::kRowGap) + panel::kControlHeight;
}

void HomeLayoutActivity::buildContent(Screen& screen, const int x, int y, const int width) {
  auto& frame = screen.frame();
  const auto heading = [&](const char* text) {
    renderer.drawText(UI_10_FONT_ID, x, y, text, true, EpdFontFamily::BOLD);
    y += panel::kLabelHeight;
  };

  // Icon size: the Home icon at each size on its squircle.
  heading(tr(STR_ICON_SIZE));
  panel::tiles(
      renderer, frame, x, y, width, 4, iconIndex(), ACTION_ICON, UI_10_FONT_ID,
      [&](const int cx, const int top, const bool, const int i) {
        const int plate = kIconPx[i] + 14 | 1;
        freeink::ui::GfxRendererTarget target(renderer);
        freeink::ui::fillSquircle(target,
                                  freeink::ui::Rect{static_cast<int16_t>(cx - plate / 2),
                                                    static_cast<int16_t>(top + (panel::kPreviewHeight - plate) / 2),
                                                    static_cast<int16_t>(plate), static_cast<int16_t>(plate)},
                                  freeink::ui::Paint::dither(freeink::ui::Color::LightGray));
        if (const freeink::Icon* icon = homeIcon(UIIcon::Apps, kIconPx[i])) {
          drawLucideIcon(renderer, *icon, cx - kIconPx[i] / 2,
                         top + (panel::kPreviewHeight - kIconPx[i]) / 2);
        }
      },
      [](const int i) { return kIconLabel[i]; });
  y += panel::kTileHeight + panel::kRowGap;

  // Grid: columns x rows of small squares.
  heading(tr(STR_HOME_GRID));
  panel::tiles(
      renderer, frame, x, y, width, 4, SETTINGS.homeGrid, ACTION_GRID, UI_10_FONT_ID,
      [&](const int cx, const int top, const bool on, const int i) {
        constexpr int cell = 11;
        constexpr int gap = 4;
        const int columns = kGridColumns[i];
        const int rows = kGridRows[i];
        const int left = cx - (columns * cell + (columns - 1) * gap) / 2;
        const int first = top + (panel::kPreviewHeight - (rows * cell + (rows - 1) * gap)) / 2;
        for (int row = 0; row < rows; ++row) {
          for (int column = 0; column < columns; ++column) {
            renderer.fillRoundedRect(left + column * (cell + gap), first + row * (cell + gap), cell, cell, 3,
                                     on ? Color::White : Color::Black);
          }
        }
      },
      [](const int i) { return kGridLabel[i]; });
  y += panel::kTileHeight + panel::kRowGap;

  panel::switchButton(renderer, frame, x, y, width, tr(STR_SHOW_NAMES), SETTINGS.homeShowNames != 0, ACTION_NAMES,
                      UI_10_FONT_ID);
}

void HomeLayoutActivity::onChoice(const freeink::ui::ActionId action, const int value) {
  if (action == ACTION_ICON && value >= 0 && value < 4) {
    SETTINGS.homeIconSize = kIconValue[value];
  } else if (action == ACTION_GRID && value >= 0 && value < 4) {
    SETTINGS.homeGrid = static_cast<uint8_t>(value);
  } else if (action == ACTION_NAMES) {
    SETTINGS.homeShowNames = SETTINGS.homeShowNames ? 0 : 1;
  }
  requestUpdate();
}
