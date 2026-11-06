#include "VegaTheme.h"

#include <GfxRenderer.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <I18n.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "RecentBooksStore.h"
#include "activities/Activity.h"
#include "activities/apps/AppRegistry.h"
#include "components/TouchRegistry.h"
#include "components/LevelBox.h"
#include "components/UITheme.h"
#include "CrossPointSettings.h"
#include "components/UiAppHelpers.h"
#include "components/icons/homeIconSet.h"
#include "fontIds.h"
#include "network/OnlineWifi.h"

namespace {
constexpr int kCardPadding = 12;
constexpr int kCardRadius = 16;
constexpr int kCoverRadius = 10;
constexpr int kCardTextGap = 16;
constexpr int kBarHeight = 10;
constexpr int kDotsHeight = 28;
constexpr int kHeaderIconSize = 24;
constexpr int kSideMargin = 16;
int coverWidth = 0;

// Rounds the corners of the picture drawn at x, y by painting what lies outside a
// quarter circle in the colour behind it (the dither pattern is the same everywhere).
void roundCorners(const GfxRenderer& renderer, const int x, const int y, const int width, const int height,
                  const int radius, const Color behind) {
  for (int row = 0; row < radius; ++row) {
    const float dy = static_cast<float>(radius - row) - 0.5f;
    const int inset = radius - static_cast<int>(std::sqrt(static_cast<float>(radius * radius) - dy * dy));
    if (inset <= 0) continue;
    for (const int edgeY : {y + row, y + height - 1 - row}) {
      renderer.fillRectDither(x, edgeY, inset, 1, behind);
      renderer.fillRectDither(x + width - inset, edgeY, inset, 1, behind);
    }
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
  // The Home header band ends topPadding below the tile's top, and is drawn after the cached card is
  // restored: start the card under it, or its top rows are painted over.
  const int cardY = rect.y + metrics.topPadding;
  const int cardX = metrics.contentSidePadding;
  const int cardW = rect.width - 2 * metrics.contentSidePadding;
  const int cardH = metrics.homeCoverHeight + 2 * kCardPadding;
  const int coverX = cardX + kCardPadding;
  const int coverY = cardY + kCardPadding;
  TouchRegistry::getInstance().add(Rect{cardX, cardY, cardW, cardH}, 0, TouchRegistry::Cover);

  if (coverWidth == 0) coverWidth = metrics.homeCoverHeight * 2 / 3;
  if (!coverRendered) {
    // The card and the cover are drawn once and kept in the snapshot; the text goes over it.
    renderer.fillRoundedRect(cardX, cardY, cardW, cardH, kCardRadius, Color::LightGray);
    coverWidth = drawCoverThumb(renderer, book, coverX, coverY, metrics.homeCoverHeight, coverWidth, false);
    // The corner cut-outs must continue the card's own dither pattern, which is anchored to its corner.
    renderer.setDitherOrigin(cardX, cardY);
    roundCorners(renderer, coverX, coverY, coverWidth, metrics.homeCoverHeight, kCoverRadius, Color::LightGray);
    renderer.setDitherOrigin(0, 0);
    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  }

  // Only a card picked with the buttons gets an outline.
  if (selectorIndex == 0) renderer.drawRoundedRect(cardX, cardY, cardW, cardH, 3, kCardRadius, true);

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
    renderer.fillRoundedRect(textX, barY, textW, kBarHeight, kBarHeight / 2, Color::White);
    const int filled = std::clamp(static_cast<int>(progressPercent / 100.0f * textW), 0, textW);
    if (filled > 0) {
      const int fillWidth = std::max(filled, kBarHeight);
      renderer.fillRoundedRect(textX, barY, fillWidth, kBarHeight, kBarHeight / 2, Color::Black);
    }
  }
}

int VegaTheme::columns() { return SETTINGS.homeGrid >= 2 ? 3 : 4; }
int VegaTheme::rows() { return SETTINGS.homeGrid == 1 || SETTINGS.homeGrid == 2 ? 4 : 3; }

void VegaTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                const std::function<const char*(int index)>& buttonLabel,
                                const std::function<UIIcon(int index)>& rowIcon) const {
  const int pages = std::max(1, pageCount(buttonCount));
  const int page = std::clamp(homePage, 0, pages - 1);
  const int columnCount = columns();
  const int cellW = (rect.width - 2 * kSideMargin) / columnCount;
  const int cellH = (rect.height - kDotsHeight) / rows();
  const int labelH = renderer.getLineHeight(UI_10_FONT_ID);
  const bool showNames = SETTINGS.homeShowNames != 0;
  const int namesH = showNames ? 6 + 2 * labelH : 0;  // room for a two-line name
  // The icon sizes are 32, 42 or 48 px on a squircle about twice as big (odd: see fillSquircle), kept inside
  // its cell.
  const int iconSize = SETTINGS.homeIconSize == 3 ? 24 : (SETTINGS.homeIconSize == 2 ? 48 : (SETTINGS.homeIconSize == 1 ? 42 : 32));
  const int squircleSize =
      std::max(iconSize + 15, std::min(2 * iconSize + 5, std::min(cellW - 8, cellH - namesH - 4))) | 1;
  const int contentH = squircleSize + namesH;

  const int pageSlots = pageSize();
  const int first = page * pageSlots;
  for (int i = first; i < buttonCount && i < first + pageSlots; ++i) {
    const int slot = i - first;
    const Rect cell{rect.x + kSideMargin + (slot % columnCount) * cellW, rect.y + (slot / columnCount) * cellH, cellW, cellH};
    TouchRegistry::getInstance().add(cell, i, TouchRegistry::Item);

    const int cx = cell.x + cell.width / 2;
    const int top = cell.y + std::max(0, cell.height - contentH) / 2;
    const bool selected = selectedIndex == i;
    freeink::ui::GfxRendererTarget target(renderer);
    freeink::ui::fillPlate(target, freeink::ui::Rect{static_cast<int16_t>(cx - squircleSize / 2), static_cast<int16_t>(top),
                                                         static_cast<int16_t>(squircleSize), static_cast<int16_t>(squircleSize)},
                              static_cast<freeink::ui::PlateShape>(SETTINGS.homeIconShape),
                              selected ? freeink::ui::Paint::solid(freeink::ui::Color::Black)
                                       : freeink::ui::Paint::dither(freeink::ui::Color::LightGray));
    if (rowIcon != nullptr) {
      if (const freeink::Icon* icon = homeIcon(rowIcon(i), iconSize)) {
        drawLucideIcon(renderer, *icon, cx - iconSize / 2, top + (squircleSize - iconSize) / 2, !selected);
      }
    }
    const char* label = buttonLabel != nullptr ? buttonLabel(i) : "";
    // A name that does not fit under its icon wraps to a second line.
    int lineY = top + squircleSize + 6;
    for (const auto& line : renderer.wrappedText(UI_10_FONT_ID, showNames && label ? label : "", cell.width - 4, 2)) {
      renderer.drawText(UI_10_FONT_ID, cx - renderer.getTextWidth(UI_10_FONT_ID, line.c_str()) / 2, lineY,
                        line.c_str(), true);
      lineY += labelH;
    }
  }

  if (pages > 1) {
    drawPageDots(renderer, rect.x, rect.y + rect.height - kDotsHeight + (kDotsHeight - 8) / 2, rect.width, pages, page);
  }
}

const freeink::Icon* VegaTheme::headerIcon() const {
  const std::string& name = activityManager.currentActivityName();
  if (const apps::Info* app = apps::findByActivityName(name)) return homeIcon(app->icon, kHeaderIconSize);
  struct Screen {
    const char* name;
    UIIcon icon;
  };
  static constexpr Screen kScreens[] = {
      {"FileBrowser", UIIcon::Folder},           {"FileBrowserSettings", UIIcon::Folder},
      {"Library", UIIcon::Library},              {"Settings", UIIcon::Settings},
      {"OpdsBookBrowser", UIIcon::Opds},         {"OpdsServerList", UIIcon::Opds},
      {"OpdsSettings", UIIcon::Opds},            {"Apps", UIIcon::Apps},
      {"CrossPointWebServer", UIIcon::Transfer}, {"NetworkModeSelection", UIIcon::Transfer},
      {"WifiSelection", UIIcon::Wifi},           {"BookStats", UIIcon::Chart},           {"StatsPicker", UIIcon::Chart},
      {"HighlightsHub", UIIcon::BookmarkIcon},   {"HighlightCategory", UIIcon::BookmarkIcon},
      {"SavedItemsHome", UIIcon::BookmarkIcon},
  };
  for (const Screen& screen : kScreens) {
    if (name == screen.name) return homeIcon(screen.icon, kHeaderIconSize);
  }
  return nullptr;
}

int VegaTheme::statusClockFontId() const { return UI_10_FONT_ID; }

// A battery without an outline: a rounded light-gray body, filled black to the charge, the percentage
// (without its sign) inside it, drawn white over the filled part and black over the rest. A bolt beside
// it while charging, and the Wi-Fi icon when Wi-Fi is on (crossed out while it is on but not joined).
void VegaTheme::drawHeaderStatus(const GfxRenderer& renderer, Rect rect, const bool batteryLeft,
                                 const bool showPercentage, const bool onBand) const {
  const uint16_t percentage = std::min<uint16_t>(powerManager.getBatteryPercentage(), 100);
  const int bodyW = showPercentage ? 38 : 30;
  const int bodyH = showPercentage ? 20 : 14;
  constexpr int nubW = 3;
  constexpr int gap = 8;
  const int centerY = rect.y + 11;  // where the classic icon centers on the clock's line
  const int bodyX = batteryLeft ? rect.x : rect.x + rect.width - nubW - bodyW;
  const int bodyY = centerY - bodyH / 2;
  const int radius = bodyH / 3;

  // The empty part is light gray, or white where the bar itself is that gray.
  const Color empty = onBand ? Color::White : Color::LightGray;
  renderer.fillRoundedRect(bodyX + bodyW, centerY - 4, nubW, 8, nubW / 2, empty);
  levelbox::draw(renderer, bodyX, bodyY, bodyW, bodyH, radius, (bodyW * percentage + 50) / 100,
                 showPercentage ? std::to_string(percentage).c_str() : nullptr, SMALL_FONT_ID, empty);

  // Beside the body, on the side away from the screen edge.
  int x = batteryLeft ? bodyX + bodyW + nubW + gap : bodyX - gap;
  const auto place = [&](const int width) {
    const int left = batteryLeft ? x : x - width;
    x = batteryLeft ? x + width + gap : x - width - gap;
    return left;
  };
  if (gpio.isUsbConnected() && percentage < 100) drawBatteryLightningBolt(renderer, place(8), centerY - 6, true);
  const freeink::Icon* wifi = OnlineWifi::connected() ? &icon_wifi_on_24 : (OnlineWifi::manualOn() ? &icon_wifi_off_24 : nullptr);
  if (wifi != nullptr) drawLucideIcon(renderer, *wifi, place(wifi->w), centerY - wifi->h / 2);
}
