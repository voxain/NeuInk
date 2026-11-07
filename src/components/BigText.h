#pragma once

#include <GfxRenderer.h>
#include <Utf8.h>

// A page's main number, drawn large: the UI font's glyphs at `scale256`/256 of their size, one by one (the font has
// no larger size). `y` is the top of the line; returns the line's height. Digits, "%", "--" and the like.
inline int drawBigText(const GfxRenderer& renderer, const int fontId, const char* text, const int x, const int y,
                       const int scale256 = 666, const bool bold = true) {
  const int ascent = renderer.getFontAscenderSize(fontId) * scale256 / 256;
  int penX = x;
  const auto* cursor = reinterpret_cast<const unsigned char*>(text);
  while (const uint32_t cp = utf8NextCodepoint(&cursor)) {
    penX += renderer.drawScaledCodepoint(fontId, cp, bold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR, penX,
                                         y + ascent, scale256);
  }
  return renderer.getLineHeight(fontId) * scale256 / 256;
}
