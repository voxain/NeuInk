#include "ScalableBuiltins.h"
#if CROSSINK_SCALABLE_FONTS
#include <GfxRenderer.h>
#include <Logging.h>

#include <optional>

#include "../../src/ReaderFontSizeStep.h"
#include "../../src/fontIds.h"
#include "HalScalableFont.h"
#include "ScalableAssets.generated.h"
// NeuInk: NEUINK_PERSIAN_FONTS=1 embeds the Vazirmatn supplement (about 35 KB) that lends Persian and
// Arabic letters to Bitter and Lexend Deca. Off by default in the X4 Pro build to save flash: Persian
// then uses an installed SD-card font (NotoVazir) as before 1.3.1. Other builds keep it on.
#ifndef NEUINK_PERSIAN_FONTS
#define NEUINK_PERSIAN_FONTS 1
#endif

namespace {
std::optional<HalScalableFont> faces[8];
#if NEUINK_PERSIAN_FONTS
// NeuInk: Persian letters the reader fonts lack, borrowed glyph by glyph (regular and bold).
std::optional<HalScalableFont> persianFace;
std::optional<HalScalableFont> persianBoldFace;
#endif
// NeuInk: pronunciation (IPA) letters and combining marks, the family's shared fallback.
std::optional<HalScalableFont> ipaFace;
bool ready[2] = {};
const uint8_t* const assets[] = {lexenddeca_regularOutline,    lexenddeca_boldOutline,  lexenddeca_italicOutline,
                                 lexenddeca_bolditalicOutline, bitter_regularOutline,   bitter_boldOutline,
                                 bitter_italicOutline,         bitter_bolditalicOutline};
constexpr size_t lengths[] = {sizeof(lexenddeca_regularOutline), sizeof(lexenddeca_boldOutline),
                              sizeof(lexenddeca_italicOutline),  sizeof(lexenddeca_bolditalicOutline),
                              sizeof(bitter_regularOutline),     sizeof(bitter_boldOutline),
                              sizeof(bitter_italicOutline),      sizeof(bitter_bolditalicOutline)};
constexpr int ids[2][4] = {{LEXENDDECA_10_FONT_ID, LEXENDDECA_12_FONT_ID, LEXENDDECA_14_FONT_ID, LEXENDDECA_16_FONT_ID},
                           {BITTER_10_FONT_ID, BITTER_12_FONT_ID, BITTER_14_FONT_ID, BITTER_16_FONT_ID}};
}  // namespace
int scalableBuiltinFontId(int id) {
  for (unsigned f = 0; f < 2; ++f)
    for (unsigned s = 0; s < 4; ++s)
      if (ids[f][s] == id && !ready[f]) return UI_12_FONT_ID;
  const int result = int(uint32_t(id) ^ OutlineFingerprint ^ 0x53544600u ^ HalScalableFont::renderingRevision());
  return result ? result : 1;
}
int scalableBuiltinReaderFontId(unsigned family, unsigned points) {
  if (family >= 2 || !ready[family]) return UI_12_FONT_ID;
  // Preserve existing cache IDs for the original four sizes.
  for (unsigned s = 0; s < 4; ++s)
    if (points == 10 + s * 2) return scalableBuiltinFontId(ids[family][s]);
  return scalableBuiltinFontId(0x54540000 | (family << 8) | points);
}
int scalableBuiltinDisplayFontId(GfxRenderer& renderer, const unsigned family, const unsigned style,
                                 const unsigned points) {
  static std::optional<HalScalableFont> face;
  static unsigned openIndex = ~0u;
  static int fontId = 0;
  const unsigned index = family * 4 + style;
  if (family >= 2 || style >= 4) return UI_12_FONT_ID;
  const int id = scalableBuiltinFontId(int(0x44530000u | (index << 8) | points));
  if (index == openIndex && fontId == id) return id;
  ScalableFontAccess access;
  if (index != openIndex) {
    if (fontId != 0) renderer.removeFont(fontId);
    fontId = 0;
    face.reset();
    face.emplace();
    if (!face->openMemory(assets[index], lengths[index])) {
      face.reset();
      openIndex = ~0u;
      LOG_ERR("TTF", "Built-in display face unavailable");
      return UI_12_FONT_ID;
    }
    openIndex = index;
  }
  const EpdFont* sized = face->atSize(static_cast<uint8_t>(points));
  if (sized == nullptr) return UI_12_FONT_ID;
  if (fontId != 0 && fontId != id) renderer.removeFont(fontId);
  // The one face stands in for every style: the line is drawn in the style it was opened for.
  renderer.insertFont(id, EpdFontFamily(sized, sized, sized, sized));
  fontId = id;
  return id;
}

void ensureScalableBuiltinFamily(GfxRenderer& renderer, unsigned family) {
  ScalableFontAccess access;
  if (family >= 2 || ready[family]) return;
  bool ok = true;
  for (unsigned style = 0; style < 4; ++style) {
    unsigned i = family * 4 + style;
    faces[i].emplace();
    if (!faces[i]->openMemory(assets[i], lengths[i])) {
      ok = false;
      break;
    }
  }
  if (!ok) {
    for (unsigned style = 0; style < 4; ++style) faces[family * 4 + style].reset();
    LOG_ERR("TTF", "Built-in family unavailable; using UI recovery font");
  }
  ready[family] = ok;
#if NEUINK_PERSIAN_FONTS
  if (ok && !persianFace) {
    persianFace.emplace();
    if (!persianFace->openMemory(vazirmatn_persianOutline, sizeof(vazirmatn_persianOutline))) {
      persianFace.reset();
      LOG_ERR("TTF", "Built-in Persian supplement unavailable");
    }
  }
  if (ok && !persianBoldFace) {
    persianBoldFace.emplace();
    if (!persianBoldFace->openMemory(vazirmatn_persian_boldOutline, sizeof(vazirmatn_persian_boldOutline))) {
      persianBoldFace.reset();
      LOG_ERR("TTF", "Built-in Persian bold supplement unavailable");
    }
  }
#endif
  if (ok && !ipaFace) {
    ipaFace.emplace();
    if (!ipaFace->openMemory(noto_ipaOutline, sizeof(noto_ipaOutline))) {
      ipaFace.reset();
      LOG_ERR("TTF", "Built-in IPA fallback unavailable");
    }
  }
  for (uint8_t points : SCALABLE_READER_FONT_SIZES) {
    if (ok) {
      EpdFontFamily font(faces[family * 4]->atSize(points), faces[family * 4 + 1]->atSize(points),
                         faces[family * 4 + 2]->atSize(points), faces[family * 4 + 3]->atSize(points),
                         ipaFace ? ipaFace->atSize(points) : nullptr);
#if NEUINK_PERSIAN_FONTS
      if (persianFace) {
        const EpdFont* persian = persianFace->atSize(points);
        font = font.withSupplement(persian, persianBoldFace ? persianBoldFace->atSize(points) : persian);
      }
#endif
      renderer.insertFont(scalableBuiltinReaderFontId(family, points), font);
      for (unsigned size = 0; size < 4; ++size)
        if (points == 10 + size * 2) renderer.insertFont(ids[family][size], font);
    } else {
      const auto it = renderer.getFontMap().find(UI_12_FONT_ID);
      if (it != renderer.getFontMap().end()) {
        for (unsigned size = 0; size < 4; ++size)
          if (points == 10 + size * 2) renderer.insertFont(ids[family][size], it->second);
      }
    }
  }
}
#endif
