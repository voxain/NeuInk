#pragma once

#include "components/themes/lyra/LyraTheme.h"

// Vega theme: Home with widgets (the current book's card by default) over every Home action and app as a light-gray
// squircle (black when selected) with its icon, paged like a phone's home screen (see VegaHome.h). Gray title bars
// with the app's icon; everything else is Lyra's.
namespace VegaMetrics {
constexpr ThemeMetrics makeValues() {
  ThemeMetrics v = LyraMetrics::values;
  v.homeCoverHeight = 176;
  v.homeCoverTileHeight = 217;  // the header's overlap (topPadding), the card and a gap
  v.homeMenuTopOffset = 8;
  v.homeTopPadding = 62;  // a taller status bar on Home (Lyra: 56)
  // Title bar: a light-gray band, no black underline, and more room.
  v.headerUnderlineSize = 0;
  v.headerHeight = 96;
  return v;
}

constexpr ThemeMetrics values = makeValues();
}  // namespace VegaMetrics

class VegaTheme : public LyraTheme {
 public:
  // Home itself (the widgets and the app grid) is components/themes/vega/VegaHome; the theme gives it the greeting.
  int homeGreetingHeight(GfxRenderer& renderer) const override;
  // "Good morning" and so on, under Home's status bar (rect.y: Home's top padding).
  void drawGreeting(GfxRenderer& renderer, Rect rect) const;

  // Title bars of the apps: light-gray band, the app's icon before its title, a little taller.
  bool headerBand() const override { return true; }
  const freeink::Icon* headerIcon() const override;
  int headerExtraHeight() const override { return 14; }
  bool selectionOnlyForButtons() const override { return true; }
  void drawHeaderStatus(const GfxRenderer& renderer, Rect batteryRect, bool batteryLeft, bool showPercentage,
                        bool onBand) const override;
  int statusClockFontId() const override;
};
