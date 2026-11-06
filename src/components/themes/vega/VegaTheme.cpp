#include "VegaTheme.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "RecentBooksStore.h"
#include "components/TouchRegistry.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace {
constexpr int kCardPadding = 12;
constexpr int kCardRadius = 14;
constexpr int kCardTextGap = 16;
constexpr int kBarHeight = 10;
constexpr int kSquircleSize = 68;
constexpr int kIconSize = 32;
constexpr int kDotsHeight = 28;
constexpr int kSideMargin = 16;
int coverWidth = 0;

// A squircle is the superellipse |x|^4 + |y|^4 = 1: rounder than a rounded square at
// the corners, flatter on the sides. Filled one pixel row at a time; light gray is
// a dither, so it shows on every panel.
void fillSquircle(const GfxRenderer& renderer, const int cx, const int cy, const int diameter, const Color color) {
  const float radius = static_cast<float>(diameter) / 2.0f;
  for (int dy = -diameter / 2; dy < diameter / 2; ++dy) {
    const float t = (static_cast<float>(dy) + 0.5f) / radius;
    const float t2 = t * t;
    const int half = static_cast<int>(std::lround(radius * std::sqrt(std::sqrt(std::max(0.0f, 1.0f - t2 * t2)))));
    renderer.fillRectDither(cx - half, cy + dy, 2 * half, 1, color);
  }
}
}  // namespace

void VegaTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                     int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                     bool& bufferRestored, const std::function<bool()>& storeCoverBuffer,
                                     const BookReadingStats*, float progressPercent, const GlobalReadingStats*,
                                     const char*) const {
  (void)bufferRestored;
  if (recentBooks.empty()) {
    drawEmptyRecents(renderer, rect);
    return;
  }
  const auto& metrics = VegaMetrics::values;
  const RecentBook& book = recentBooks[0];
  const int cardX = metrics.contentSidePadding;
  const int cardW = rect.width - 2 * metrics.contentSidePadding;
  const int cardH = metrics.homeCoverHeight + 2 * kCardPadding;
  const int coverX = cardX + kCardPadding;
  const int coverY = rect.y + kCardPadding;
  TouchRegistry::getInstance().add(Rect{cardX, rect.y, cardW, cardH}, 0, TouchRegistry::Cover);

  if (coverWidth == 0) coverWidth = metrics.homeCoverHeight * 2 / 3;
  if (!coverRendered) {
    coverWidth = drawCoverThumb(renderer, book, coverX, coverY, metrics.homeCoverHeight, coverWidth);
    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  }

  // The selected card is outlined heavier.
  const bool selected = selectorIndex == 0;
  renderer.drawRoundedRect(cardX, rect.y, cardW, cardH, selected ? 3 : 1, kCardRadius, true);

  const int textX = coverX + coverWidth + kCardTextGap;
  const int textW = cardX + cardW - kCardPadding - textX;
  int y = coverY;
  const int titleLineHeight = renderer.getLineHeight(UI_12_FONT_ID);
  for (const auto& line : renderer.wrappedText(UI_12_FONT_ID, book.title.c_str(), textW, 3, EpdFontFamily::BOLD)) {
    renderer.drawText(UI_12_FONT_ID, textX, y, line.c_str(), true, EpdFontFamily::BOLD);
    y += titleLineHeight;
  }
  if (!book.author.empty()) {
    renderer.drawText(UI_10_FONT_ID, textX, y + renderer.getLineHeight(UI_10_FONT_ID) / 3,
                      renderer.truncatedText(UI_10_FONT_ID, book.author.c_str(), textW).c_str(), true);
  }

  if (progressPercent >= 0.0f) {
    // The percentage over a pill bar, at the foot of the card.
    const int barY = coverY + metrics.homeCoverHeight - kBarHeight;
    char label[16];
    snprintf(label, sizeof(label), "%.0f%%", progressPercent);
    renderer.drawText(UI_10_FONT_ID, textX, barY - renderer.getLineHeight(UI_10_FONT_ID) - 4, label, true,
                      EpdFontFamily::BOLD);
    renderer.fillRoundedRect(textX, barY, textW, kBarHeight, kBarHeight / 2, Color::LightGray);
    const int filled = std::clamp(static_cast<int>(progressPercent / 100.0f * textW), 0, textW);
    if (filled > 0) {
      const int fillWidth = std::max(filled, kBarHeight);
      renderer.fillRoundedRect(textX, barY, fillWidth, kBarHeight, kBarHeight / 2, Color::Black);
    }
  }
}

void VegaTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                const std::function<const char*(int index)>& buttonLabel,
                                const std::function<UIIcon(int index)>& rowIcon) const {
  const int pages = std::max(1, pageCount(buttonCount));
  const int page = std::clamp(homePage, 0, pages - 1);
  const int cellW = (rect.width - 2 * kSideMargin) / kColumns;
  const int cellH = (rect.height - kDotsHeight) / kRows;
  const int labelH = renderer.getLineHeight(UI_10_FONT_ID);
  const int contentH = kSquircleSize + 6 + labelH;

  const int first = page * kPageSize;
  for (int i = first; i < buttonCount && i < first + kPageSize; ++i) {
    const int slot = i - first;
    const Rect cell{rect.x + kSideMargin + (slot % kColumns) * cellW, rect.y + (slot / kColumns) * cellH, cellW, cellH};
    TouchRegistry::getInstance().add(cell, i, TouchRegistry::Item);

    const int cx = cell.x + cell.width / 2;
    const int top = cell.y + std::max(0, cell.height - contentH) / 2;
    const bool selected = selectedIndex == i;
    fillSquircle(renderer, cx, top + kSquircleSize / 2, kSquircleSize, selected ? Color::Black : Color::LightGray);
    if (rowIcon != nullptr) {
      if (const freeink::Icon* icon = iconForName(rowIcon(i), kIconSize)) {
        drawLucideIcon(renderer, *icon, cx - kIconSize / 2, top + (kSquircleSize - kIconSize) / 2, !selected);
      }
    }
    const char* label = buttonLabel != nullptr ? buttonLabel(i) : "";
    const std::string text = renderer.truncatedText(UI_10_FONT_ID, label ? label : "", cell.width - 4);
    renderer.drawText(UI_10_FONT_ID, cx - renderer.getTextWidth(UI_10_FONT_ID, text.c_str()) / 2,
                      top + kSquircleSize + 6, text.c_str(), true);
  }

  // Page dots: the current page is a pill.
  if (pages > 1) {
    constexpr int dot = 8;
    constexpr int pill = 22;
    constexpr int gap = 8;
    const int total = (pages - 1) * dot + pill + (pages - 1) * gap;
    int x = rect.x + (rect.width - total) / 2;
    const int y = rect.y + rect.height - kDotsHeight + (kDotsHeight - dot) / 2;
    for (int p = 0; p < pages; ++p) {
      const bool current = p == page;
      renderer.fillRoundedRect(x, y, current ? pill : dot, dot, dot / 2, current ? Color::Black : Color::LightGray);
      x += (current ? pill : dot) + gap;
    }
  }
}
