#pragma once
#if CROSSINK_SCALABLE_FONTS
class GfxRenderer;
void ensureScalableBuiltinFamily(GfxRenderer& renderer, unsigned family);
int scalableBuiltinFontId(int legacyId);
int scalableBuiltinReaderFontId(unsigned family, unsigned points);
// One built-in face (family 0 Lexend Deca, 1 Bitter; style 0 regular .. 3 bold italic) at one size, for a single
// large line such as the Vega greeting: a small part of the font arena the reader's fonts share, where a whole
// family in every reader size can leave too little of it for an SD-card font. UI_12_FONT_ID if it cannot open.
int scalableBuiltinDisplayFontId(GfxRenderer& renderer, unsigned family, unsigned style, unsigned points);
#endif
