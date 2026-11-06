#pragma once

#include <GfxRenderer.h>

#include <algorithm>

// A rounded box that shows a level: a light-gray (or `empty`) track, filled black up to `fillWidth`, with a
// caption in it that is white over the fill and black over the rest. The quick settings sliders and the status
// bar's battery are both this. The right end of a part-filled box is flat, so it reads as a level.
namespace levelbox {

inline void draw(const GfxRenderer& renderer, const int x, const int y, const int width, const int height,
                 const int radius, const int fillWidth, const char* caption, const int fontId,
                 const Color empty = Color::LightGray) {
  const int fill = std::clamp(fillWidth, 0, width);
  renderer.fillRoundedRect(x, y, width, height, radius, empty);
  if (fill > 0) {
    const bool full = fill >= width - radius;
    renderer.fillRoundedRect(x, y, fill, height, radius, true, full, true, full, Color::Black);
  }
  if (caption == nullptr || caption[0] == '\0') return;
  const int textX = x + (width - renderer.getTextWidth(fontId, caption, EpdFontFamily::BOLD)) / 2;
  const int textY = y + (height - renderer.getLineHeight(fontId)) / 2;
  if (fill > 0) {
    renderer.beginTextClip(x, y, fill, height);
    renderer.drawText(fontId, textX, textY, caption, false, EpdFontFamily::BOLD);
    renderer.endTextClip();
  }
  if (fill < width) {
    renderer.beginTextClip(x + fill, y, width - fill, height);
    renderer.drawText(fontId, textX, textY, caption, true, EpdFontFamily::BOLD);
    renderer.endTextClip();
  }
}

}  // namespace levelbox
