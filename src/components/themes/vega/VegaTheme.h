#pragma once

#include "components/themes/lyra/LyraTheme.h"

// Vega theme: the last opened book as a card on top (cover, title, author and a
// pill progress bar), under it every Home action and app as a light-gray squircle (black when selected) with its
// icon, paged like a phone's home screen. Everything else is Lyra's.
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
  // The grid's shape comes from the Home layout pane (columns x rows: 4x3, 4x4, 3x4, 3x3).
  static int columns();
  static int rows();
  static int pageSize() { return columns() * rows(); }

  // The page the grid shows. Home sets it before drawing, from the selection or a swipe.
  static void setHomePage(int page) { homePage = page; }
  static int pageOf(int index) { return index / pageSize(); }
  static int pageCount(int itemCount) { return (itemCount + pageSize() - 1) / pageSize(); }

  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           const std::function<bool()>& storeCoverBuffer, const BookReadingStats* stats = nullptr,
                           float progressPercent = -1.0f, const GlobalReadingStats* globalStats = nullptr,
                           const char* currentChapterTitle = nullptr) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<const char*(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
  int homeGreetingHeight(GfxRenderer& renderer) const override;

 // Title bars of the apps: light-gray band, the app's icon before its title, a little taller.
  bool headerBand() const override { return true; }
  const freeink::Icon* headerIcon() const override;
  int headerExtraHeight() const override { return 14; }
  bool selectionOnlyForButtons() const override { return true; }
  void drawHeaderStatus(const GfxRenderer& renderer, Rect batteryRect, bool batteryLeft, bool showPercentage,
                        bool onBand) const override;
  int statusClockFontId() const override;

 private:
  void drawGreeting(GfxRenderer& renderer, Rect rect) const;
  static inline int homePage = 0;
};
