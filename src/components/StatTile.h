#pragma once

#include <GfxRenderer.h>

#include <string>

#include "fontIds.h"

// A figure in bold with its name under it, centred on a rounded tile: the reading stats pages and Home's widgets.
inline void drawStatTile(const GfxRenderer& renderer, const int x, const int y, const int width, const int height,
                         const char* value, const char* label, const Color background, const int radius = 14) {
  renderer.fillRoundedRect(x, y, width, height, radius, background);
  const int valueH = renderer.getLineHeight(UI_12_FONT_ID);
  const int labelH = renderer.getLineHeight(SMALL_FONT_ID);
  const int top = y + (height - valueH - 4 - labelH) / 2;
  const auto centred = [&](const int font, const int lineY, const char* text, const EpdFontFamily::Style style) {
    const std::string fitted = renderer.truncatedText(font, text, width - 8, style);
    renderer.drawText(font, x + (width - renderer.getTextWidth(font, fitted.c_str(), style)) / 2, lineY,
                      fitted.c_str(), true, style);
  };
  centred(UI_12_FONT_ID, top, value, EpdFontFamily::BOLD);
  centred(SMALL_FONT_ID, top + valueH + 4, label, EpdFontFamily::REGULAR);
}
