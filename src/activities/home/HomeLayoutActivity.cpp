#include "HomeLayoutActivity.h"

#include <I18n.h>

#include <algorithm>
#include <cstdio>

#include "CrossPointSettings.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace {
constexpr freeink::ui::ActionId ACTION_ICON = PanelActivity::ACTION_FIRST_CONTROL;
constexpr freeink::ui::ActionId ACTION_SHAPE = PanelActivity::ACTION_FIRST_CONTROL + 1;
constexpr freeink::ui::ActionId ACTION_COLUMNS = PanelActivity::ACTION_FIRST_CONTROL + 2;
constexpr freeink::ui::ActionId ACTION_ROWS = PanelActivity::ACTION_FIRST_CONTROL + 3;
constexpr freeink::ui::ActionId ACTION_NAMES = PanelActivity::ACTION_FIRST_CONTROL + 4;
constexpr freeink::ui::ActionId ACTION_GREETING = PanelActivity::ACTION_FIRST_CONTROL + 5;
constexpr freeink::ui::ActionId ACTION_GRAY = PanelActivity::ACTION_FIRST_CONTROL + 6;

enum Section { IconSize, Shape, Grid, Names, Greeting, TrueGray, kSectionCount };

// The options in the order shown, as the values stored in homeIconSize (0 = 32 px, the default).
constexpr int kIconPx[] = {24, 32, 42, 48};
constexpr uint8_t kIconValue[] = {3, 0, 1, 2};
constexpr const char* kIconLabel[] = {"24", "32", "42", "48"};

// The plate shapes in the order shown, as stored in homeIconShape.
constexpr StrId kShapeLabel[] = {StrId::STR_SHAPE_SQUIRCLE, StrId::STR_SHAPE_ROUNDED, StrId::STR_SHAPE_CIRCLE,
                                 StrId::STR_SHAPE_HEXAGON};

int iconIndex() {
  for (int i = 0; i < 4; ++i) {
    if (kIconValue[i] == SETTINGS.homeIconSize) return i;
  }
  return 1;
}

// An Apps icon of `size` px on a plate of `shape`, centred on cx in a tile's picture.
void platePreview(const GfxRenderer& renderer, const int cx, const int top, const int plate, const int shape,
                  const int size) {
  freeink::ui::GfxRendererTarget target(renderer);
  freeink::ui::fillPlate(target,
                         freeink::ui::Rect{static_cast<int16_t>(cx - plate / 2),
                                           static_cast<int16_t>(top + (panel::kPreviewHeight - plate) / 2),
                                           static_cast<int16_t>(plate), static_cast<int16_t>(plate)},
                         static_cast<freeink::ui::PlateShape>(shape),
                         freeink::ui::Paint::dither(freeink::ui::Color::LightGray));
  if (const freeink::Icon* icon = homeIcon(UIIcon::Apps, size)) {
    drawLucideIcon(renderer, *icon, cx - size / 2, top + (panel::kPreviewHeight - size) / 2);
  }
}
}  // namespace

HomeLayoutActivity::HomeLayoutActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : PanelActivity(renderer, mappedInput, "HomeLayout") {}

void HomeLayoutActivity::onExit() {
  SETTINGS.saveToFile();
  PanelActivity::onExit();
}

int HomeLayoutActivity::sectionCount() const { return kSectionCount; }

int HomeLayoutActivity::sectionHeight(const int section) const {
  if (section <= Shape) return panel::kLabelHeight + panel::kTileHeight + panel::kRowGap;
  if (section == Grid) return panel::kLabelHeight + panel::kControlHeight + panel::kRowGap;
  return panel::kControlHeight + panel::kRowGap / 2;
}

// About two thirds of the screen: the icons, their shape and the grid together, and room above to tap it closed.
int HomeLayoutActivity::maxContentHeight() const {
  return renderer.getScreenHeight() * 65 / 100 - panel::kSide - topInset() - panel::kPad;
}

const freeink::Icon* HomeLayoutActivity::cornerIcon() const { return &icon_notes_24; }

void HomeLayoutActivity::onCorner() {
  MenuResult result;
  result.action = kEditHome;
  setResult(std::move(result));
  finish();
}

void HomeLayoutActivity::buildSection(Screen& screen, const int section, const int x, int y, const int width) {
  auto& frame = screen.frame();
  char value[8];
  switch (section) {
    case IconSize:
      panel::heading(renderer, x, y, tr(STR_ICON_SIZE), UI_10_FONT_ID);
      panel::tiles(
          renderer, frame, x, y + panel::kLabelHeight, width, 4, iconIndex(), ACTION_ICON, UI_10_FONT_ID,
          [&](const int cx, const int top, const bool, const int i) {
            platePreview(renderer, cx, top, kIconPx[i] + 14 | 1, 0, kIconPx[i]);
          },
          [](const int i) { return kIconLabel[i]; });
      break;
    case Shape:
      panel::heading(renderer, x, y, tr(STR_ICON_SHAPE), UI_10_FONT_ID);
      panel::tiles(
          renderer, frame, x, y + panel::kLabelHeight, width, 4, SETTINGS.homeIconShape, ACTION_SHAPE, UI_10_FONT_ID,
          [&](const int cx, const int top, const bool, const int i) { platePreview(renderer, cx, top, 49, i, 24); },
          [](const int i) { return I18N.get(kShapeLabel[i]); });
      break;
    case Grid: {
      // Columns and rows side by side: one is chosen with the other in view.
      const int half = (width - 12) / 2;
      panel::heading(renderer, x, y, tr(STR_HOME_COLUMNS), UI_10_FONT_ID);
      panel::heading(renderer, x + half + 12, y, tr(STR_HOME_ROWS), UI_10_FONT_ID);
      snprintf(value, sizeof(value), "%d", SETTINGS.homeColumns);
      panel::stepper(renderer, frame, x, y + panel::kLabelHeight, half, value, ACTION_COLUMNS, UI_10_FONT_ID);
      snprintf(value, sizeof(value), "%d", SETTINGS.homeRows);
      panel::stepper(renderer, frame, x + half + 12, y + panel::kLabelHeight, half, value, ACTION_ROWS, UI_10_FONT_ID);
      break;
    }
    case Names:
      panel::toggleRow(renderer, frame, x, y, width, tr(STR_SHOW_NAMES), SETTINGS.homeShowNames != 0, ACTION_NAMES,
                       UI_10_FONT_ID);
      break;
    case Greeting:
      panel::toggleRow(renderer, frame, x, y, width, tr(STR_HOME_GREETING), SETTINGS.homeGreeting != 0,
                       ACTION_GREETING, UI_10_FONT_ID);
      break;
    case TrueGray:
      panel::toggleRow(renderer, frame, x, y, width, tr(STR_TRUE_GRAY), SETTINGS.homeTrueGray != 0, ACTION_GRAY,
                       UI_10_FONT_ID);
      break;
    default:
      break;
  }
}

void HomeLayoutActivity::onChoice(const freeink::ui::ActionId action, const int value) {
  if (action == ACTION_ICON && value >= 0 && value < 4) {
    SETTINGS.homeIconSize = kIconValue[value];
  } else if (action == ACTION_SHAPE && value >= 0 && value < 4) {
    SETTINGS.homeIconShape = static_cast<uint8_t>(value);
  } else if (action == ACTION_COLUMNS) {
    SETTINGS.homeColumns = static_cast<uint8_t>(std::clamp(SETTINGS.homeColumns + value, 2, 6));
  } else if (action == ACTION_ROWS) {
    SETTINGS.homeRows = static_cast<uint8_t>(std::clamp(SETTINGS.homeRows + value, 1, 8));
  } else if (action == ACTION_NAMES) {
    SETTINGS.homeShowNames = SETTINGS.homeShowNames ? 0 : 1;
  } else if (action == ACTION_GREETING) {
    SETTINGS.homeGreeting = SETTINGS.homeGreeting ? 0 : 1;
  } else if (action == ACTION_GRAY) {
    SETTINGS.homeTrueGray = SETTINGS.homeTrueGray ? 0 : 1;
  }
  requestUpdate();
}
