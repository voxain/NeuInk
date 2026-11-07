#pragma once

#include <GfxRenderer.h>

#include <algorithm>

// A rounded box that shows a level: a light-gray (or `empty`) track, filled black up to `fillWidth`, with a
// caption in it that is white over the fill and black over the rest. The quick settings sliders and the batteries
// (the header's and the reader's status bar's) are all this. The right end of a part-filled box is flat, so it reads as a level.
namespace levelbox {

inline void draw(const GfxRenderer& renderer, const int x, const int y, const int width, const int height,
                 const int radius, const int fillWidth, const char* caption, const int fontId,
                 const Color empty = Color::LightGray, const Color fillColor = Color::Black) {
  const int fill = std::clamp(fillWidth, 0, width);
  renderer.fillRoundedRect(x, y, width, height, radius, empty);
  if (fill > 0) {
    const bool full = fill >= width - radius;
    renderer.fillRoundedRect(x, y, fill, height, radius, true, full, true, full, fillColor);
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

// A battery: the box and a small nub at its right end, vertically centred on centerY. The nub is the empty colour.
inline void battery(const GfxRenderer& renderer, const int x, const int centerY, const int bodyWidth,
                    const int bodyHeight, const int nubHeight, const int percentage, const char* caption,
                    const int fontId, const Color empty = Color::LightGray, const Color fillColor = Color::Black) {
  constexpr int nubWidth = 3;
  renderer.fillRoundedRect(x + bodyWidth, centerY - nubHeight / 2, nubWidth, nubHeight, nubWidth / 2, empty);
  draw(renderer, x, centerY - bodyHeight / 2, bodyWidth, bodyHeight, bodyHeight / 3,
       (bodyWidth * std::clamp(percentage, 0, 100) + 50) / 100, caption, fontId, empty, fillColor);
}

}  // namespace levelbox
