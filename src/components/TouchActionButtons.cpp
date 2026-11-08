#include "TouchActionButtons.h"

#include <GfxRenderer.h>

#include <algorithm>

#include "fontIds.h"

namespace TouchActionButtons {

Layout vertical(const Rect container, const uint8_t count, const int buttonHeight, const int gap) {
  Layout result;
  result.container = container;
  result.count = std::min<uint8_t>(count, static_cast<uint8_t>(kMaxButtons));
  if (result.count == 0 || container.width <= 0 || container.height <= 0) return result;

  const int safeHeight = std::max(1, buttonHeight);
  const int safeGap = std::max(0, gap);
  const int totalHeight = safeHeight * result.count + safeGap * (result.count - 1);
  const int firstY = container.y + std::max(0, container.height - totalHeight);
  for (uint8_t i = 0; i < result.count; ++i) {
    result.buttons[i] = Rect{container.x, firstY + i * (safeHeight + safeGap), container.width, safeHeight};
  }
  return result;
}

int indexAt(const Layout& layout, const int x, const int y) {
  for (uint8_t i = 0; i < layout.count; ++i) {
    const Rect& button = layout.buttons[i];
    if (x >= button.x && x < button.x + button.width && y >= button.y && y < button.y + button.height) {
      return i;
    }
  }
  return -1;
}

void drawButton(const GfxRenderer& renderer, const int x, const int y, const int w, const int h, const char* label,
                const bool primary, const bool selected, int fontId) {
  if (fontId == 0) fontId = UI_10_FONT_ID;
  // The look of the shared UI components: light gray, black when picked, no outline.
  renderer.fillRoundedRect(x, y, w, h, 14, selected ? Color::Black : Color::LightGray);
  if (label == nullptr) label = "";
  const EpdFontFamily::Style style = primary ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  const int textWidth = renderer.getTextWidth(fontId, label, style);
  renderer.drawText(fontId, x + (w - textWidth) / 2, y + (h - renderer.getLineHeight(fontId)) / 2, label, !selected,
                    style);
}

void draw(const GfxRenderer& renderer, const Layout& layout, const char* const* labels, const int primaryIndex,
          const int selectedIndex, const int fontId) {
  if (labels == nullptr) return;
  for (uint8_t i = 0; i < layout.count; ++i) {
    const Rect& b = layout.buttons[i];
    drawButton(renderer, b.x, b.y, b.width, b.height, labels[i], static_cast<int>(i) == primaryIndex,
               static_cast<int>(i) == selectedIndex, fontId);
  }
}

}  // namespace TouchActionButtons
