#include "StorageActivity.h"

#include <Arduino.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "MappedInputManager.h"
#include "components/BigText.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"
#include "util/ByteSize.h"
#include "util/StorageScan.h"

namespace {
constexpr int kSide = 16;
constexpr int kBarHeight = 56;
constexpr int kBarRadius = 16;
constexpr int kBorder = 2;
constexpr int kRowHeight = 44;
constexpr int kRowGap = 6;
constexpr int kSwatch = 26;
constexpr unsigned long kSpinMs = 600;

// A category in the legend: its name, icon and the gray it is in the bar. The card only has four grays, so two
// categories share one; they never sit next to each other in the bar.
struct Entry {
  StrId name;
  const freeink::Icon* icon;
  Color gray;
};
constexpr Entry kEntries[StorageScan::kCategories] = {
    {StrId::STR_STORAGE_BOOKS, &icon_type_book_24, Color::Black},
    {StrId::STR_STORAGE_IMAGES, &icon_type_image_24, Color::DarkGray},
    {StrId::STR_STORAGE_RESOURCES, &icon_dictionaries_24, Color::LightGray},
    {StrId::STR_STORAGE_APP_DATA, &icon_database_24, Color::Black},
    {StrId::STR_OTHER, &icon_type_file_24, Color::DarkGray},
};
}  // namespace

StorageActivity::StorageActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Storage", renderer, mappedInput) {}

void StorageActivity::onEnter() {
  Activity::onEnter();
  StorageScan::start();
  requestUpdate();
}

void StorageActivity::onExit() {
  StorageScan::stop();  // it ends by itself; nothing waits for it
  Activity::onExit();
}

void StorageActivity::loop() {
  if (StorageScan::takeFinished()) {
    // A scan that an earlier visit cut short is not an answer: ask again.
    if (!StorageScan::result().complete) StorageScan::start();
    requestUpdate();
  }
  // A turn of the spinner while it scans: a frame every so often, the panel cannot do more.
  if (StorageScan::running() && millis() - lastSpinMs >= kSpinMs) {
    lastSpinMs = millis();
    requestUpdate();
  }
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
  }
}

void StorageActivity::render(RenderLock&&) {
  renderer.clearScreen();
  const Rect header = TouchHeaderBackButton::drawPageHeader(renderer, mappedInput, tr(STR_STORAGE));
  const int width = renderer.getScreenWidth();
  const StorageScan::Result& scan = StorageScan::result();
  const bool scanning = StorageScan::running() || !scan.complete;
  const uint64_t total = Storage.totalBytes();
  int y = header.y + header.height + 24;

  // How much is used, big ("--" until it is known), and of how much.
  const int bigHeight = drawBigText(renderer, UI_12_FONT_ID, scanning ? "--" : formatByteSize(scan.used).c_str(), kSide, y);
  char text[48];
  snprintf(text, sizeof(text), tr(STR_STORAGE_USED_OF), formatByteSize(total).c_str());
  renderer.drawText(UI_10_FONT_ID, kSide, y + bigHeight, text, true);
  y += bigHeight + renderer.getLineHeight(UI_10_FONT_ID) + 20;

  // The bar: the categories side by side in their grays with a white line between them, the track's corners cut
  // round, then its outline. While scanning the track is gray with the spinner on it, turned a quarter further
  // with every frame.
  const int barWidth = width - 2 * kSide;
  if (scanning) {
    renderer.fillRoundedRect(kSide, y, barWidth, kBarHeight, kBarRadius, Color::LightGray);
    drawLucideIcon(renderer, icon_loader_32, kSide + (barWidth - 32) / 2, y + (kBarHeight - 32) / 2, true,
                   static_cast<freeink::ui::Rotation>(spinStep++ % 4));
  } else if (total > 0) {
    const int innerX = kSide + kBorder;
    const int innerWidth = barWidth - 2 * kBorder;
    const int innerHeight = kBarHeight - 2 * kBorder;
    uint64_t before = 0;
    int last = -1;
    for (int i = 0; i < StorageScan::kCategories; ++i) {
      if (scan.bytes[i] > 0) last = i;
    }
    for (int i = 0; i < StorageScan::kCategories; ++i) {
      if (scan.bytes[i] == 0) continue;
      const int from = static_cast<int>(before * innerWidth / total);
      before += scan.bytes[i];
      const int to = std::min(innerWidth, std::max(from + 3, static_cast<int>(before * innerWidth / total)));
      renderer.fillRectDither(innerX + from, y + kBorder, to - from, innerHeight, kEntries[i].gray);
      if (i != last) renderer.fillRect(innerX + to - 1, y + kBorder, 2, innerHeight, false);
    }
    // Flat segments overflow the track's round corners: paint what lies outside the curve white again.
    for (int row = 0; row < kBarRadius; ++row) {
      const double dy = kBarRadius - row - 0.5;
      const int cut = static_cast<int>(std::ceil(kBarRadius - std::sqrt(kBarRadius * kBarRadius - dy * dy)));
      for (const int top : {y + row, y + kBarHeight - 1 - row}) {
        renderer.fillRect(kSide, top, cut, 1, false);
        renderer.fillRect(kSide + barWidth - cut, top, cut, 1, false);
      }
    }
  }
  renderer.drawRoundedRect(kSide, y, barWidth, kBarHeight, kBorder, kBarRadius, true);
  y += kBarHeight + 22;

  // One gray row per category: its gray, icon, name and size ("--" until known), and what is left free.
  const auto row = [&](const Color gray, const freeink::Icon* icon, const char* name, const std::string& value) {
    renderer.fillRoundedRect(kSide, y, barWidth, kRowHeight, kRowHeight / 2, Color::LightGray);
    const int swatchX = kSide + 14;
    const int swatchY = y + (kRowHeight - kSwatch) / 2;
    renderer.fillRoundedRect(swatchX, swatchY, kSwatch, kSwatch, 8, gray);
    renderer.drawRoundedRect(swatchX, swatchY, kSwatch, kSwatch, kBorder, 8, true);
    drawLucideIcon(renderer, *icon, swatchX + kSwatch + 12, y + (kRowHeight - 24) / 2);
    const int textY = y + (kRowHeight - renderer.getLineHeight(UI_10_FONT_ID)) / 2;
    renderer.drawText(UI_10_FONT_ID, swatchX + kSwatch + 12 + 24 + 12, textY, name, true);
    renderer.drawText(UI_10_FONT_ID, width - kSide - 20 - renderer.getTextWidth(UI_10_FONT_ID, value.c_str(), EpdFontFamily::BOLD),
                      textY, value.c_str(), true, EpdFontFamily::BOLD);
    y += kRowHeight + kRowGap;
  };
  for (int i = 0; i < StorageScan::kCategories; ++i) {
    row(kEntries[i].gray, kEntries[i].icon, I18N.get(kEntries[i].name), scanning ? "--" : formatByteSize(scan.bytes[i]));
  }
  row(Color::White, &icon_storage_24, tr(STR_STORAGE_FREE), scanning ? "--" : formatByteSize(total > scan.used ? total - scan.used : 0));

  const auto labels = mappedInput.mapLabels(mappedInput.withBackArrow(tr(STR_BACK)), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
