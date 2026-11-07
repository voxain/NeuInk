#pragma once
#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>
#include <FreeInkUIIcon.h>

#include <atomic>
#include <cstdint>

#include "MappedInputManager.h"
#include "components/UIScale.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/icons/homeIconSet.h"

// Shared glue for activities hosting a FreeInkApp: the font-bound render
// target and the touch snapshot FreeInkApp routing consumes.

// Bind the uiScale fonts before FreeInkApp's constructor derives its theme
// metrics from the body font's line height.
inline freeink::ui::GfxRendererTarget makeUiTarget(const GfxRenderer& renderer) {
  freeink::ui::GfxRendererTarget target(renderer);
  const auto spec = uiScaleSpec();
  target.setFont(freeink::ui::GfxRendererTarget::FONT_SMALL, spec.smallFontId);
  target.setFont(freeink::ui::GfxRendererTarget::FONT_BODY, spec.bodyFontId);
  target.setFont(freeink::ui::GfxRendererTarget::FONT_TITLE, spec.titleFontId);
  return target;
}

// Activities share two static token generations rather than each retaining an
// identical ~1.5KB copy. A render task always reads the published generation;
// live configuration changes build the other generation before swapping the
// atomic pointer, so no reader can observe a partially updated token object.
namespace UiAppThemeDetail {
struct ConfigKey {
  uint8_t uiTheme = UINT8_MAX;
  uint8_t uiScale = UINT8_MAX;
  bool hasTouch = false;
  int16_t bodyLineHeight = -1;

  bool operator==(const ConfigKey& other) const {
    return uiTheme == other.uiTheme && uiScale == other.uiScale && hasTouch == other.hasTouch &&
           bodyLineHeight == other.bodyLineHeight;
  }
};

inline freeink::ui::ThemeTokens* tokenSlots() {
  static freeink::ui::ThemeTokens slots[2];
  return slots;
}

inline std::atomic<const freeink::ui::ThemeTokens*>& publishedTokens() {
  static std::atomic<const freeink::ui::ThemeTokens*> published{nullptr};
  return published;
}

inline ConfigKey& lastConfig() {
  static ConfigKey config;
  return config;
}
}  // namespace UiAppThemeDetail

template <size_t MaxInteractions, size_t MaxHandlers>
inline void applySharedUiTheme(freeink::ui::FreeInkApp<MaxInteractions, MaxHandlers>& app,
                               const freeink::ui::GfxRendererTarget& target) {
  const UiAppThemeDetail::ConfigKey config{SETTINGS.uiTheme, SETTINGS.uiScale, gpio.hasTouch(),
                                           target.lineHeight(freeink::ui::GfxRendererTarget::FONT_BODY)};
  auto& published = UiAppThemeDetail::publishedTokens();
  const auto current = published.load(std::memory_order_acquire);
  if (current == nullptr || !(config == UiAppThemeDetail::lastConfig())) {
    auto* slots = UiAppThemeDetail::tokenSlots();
    auto* next = current == &slots[0] ? &slots[1] : &slots[0];
    *next = uiThemeTokens(target);
    published.store(next, std::memory_order_release);
    UiAppThemeDetail::lastConfig() = config;
  }
  app.setThemeRef(&published);
}

// Tap release with coords, plus the raw release the tap classifier never
// reports (swipe end, drag-off) delivered off-target: nothing dispatches,
// but routing drops its pressed-element state instead of ghosting it onto
// the next render.
// Firmware UIIcon -> FreeInkUI bitmap for list rows (SDK-format icons only;
// the legacy drawIcon assets use a different bit layout). Two crisp sizes:
// 24 for single-line rows, 32 for label+subtitle rows.
inline freeink::ui::BitmapRef listIconFor(const UIIcon icon, const int size = 24) {
  const freeink::Icon* bitmap = homeIcon(icon, static_cast<uint32_t>(size));
  return bitmap != nullptr ? freeink::ui::bitmapFromIcon(*bitmap) : freeink::ui::BitmapRef{};
}

// The look of a control on the gray surfaces: a light-gray rounded button, black (white label) when picked or pressed.
inline freeink::ui::StyleSet grayButtonStyles() {
  namespace fui = freeink::ui;
  fui::StyleSet styles;
  styles.explicitlySet = true;
  styles.normal.background = fui::Paint::dither(fui::Color::LightGray);
  styles.normal.radius = 14;
  styles.selected = styles.normal;
  styles.selected.background = fui::Paint::solid(fui::Color::Black);
  styles.selected.foreground = fui::Paint::solid(fui::Color::White);
  styles.focused = styles.selected;
  styles.active = styles.selected;
  styles.disabled = styles.normal;
  return styles;
}

// Legacy GfxRenderer icon arrays are pre-rotated. Render SDK Lucide assets through
// its adapter instead so the same bitmap remains correct in every orientation.
inline void drawLucideIcon(const GfxRenderer& renderer, const freeink::Icon& icon, const int x, const int y,
                           const bool foregroundBlack = true,
                           const freeink::ui::Rotation rotation = freeink::ui::Rotation::None) {
  freeink::ui::GfxRendererTarget target(renderer);
  target.bitmap(freeink::ui::Rect{static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(icon.w),
                                  static_cast<int16_t>(icon.h)},
                freeink::ui::bitmapFromIcon(icon), freeink::ui::BitmapMode::Center,
                freeink::ui::Paint::solid(foregroundBlack ? freeink::ui::Color::Black : freeink::ui::Color::White), rotation);
}

// Scroll semantics shared by every FreeInkUI list screen: swipes move the
// viewport (topIndex) without touching the selection; button navigation moves
// the selection and pulls the viewport along just enough to keep it visible.

// Page dots: one size for all, the current page black and the others light gray, centred in the band at y (height 9).
// Odd size and anchored to their middle (fillSquircle), so a gray dot's corners dither the same on every side.
inline void drawPageDots(const GfxRenderer& renderer, const int x, const int y, const int width, const int pages,
                         const int page) {
  constexpr int dot = 9;
  constexpr int gap = 9;
  freeink::ui::GfxRendererTarget target(renderer);
  int px = x + (width - (pages * dot + (pages - 1) * gap)) / 2;
  for (int p = 0; p < pages; ++p) {
    freeink::ui::fillSquircle(target, freeink::ui::Rect{static_cast<int16_t>(px), static_cast<int16_t>(y), dot, dot},
                              p == page ? freeink::ui::Paint::solid(freeink::ui::Color::Black)
                                        : freeink::ui::Paint::dither(freeink::ui::Color::LightGray));
    px += dot + gap;
  }
}

inline int scrollListBy(const int topIndex, const int delta, const int visibleRows, const int count) {
  int maxTop = count - visibleRows;
  if (maxTop < 0) maxTop = 0;
  int next = topIndex + delta;
  if (next > maxTop) next = maxTop;
  if (next < 0) next = 0;
  return next;
}

inline int followListSelection(const int selectedIndex, const int topIndex, const int visibleRows, const int count) {
  return static_cast<int>(freeink::ui::listTopIndexFor(
      static_cast<int16_t>(selectedIndex), static_cast<uint16_t>(topIndex),
      static_cast<uint16_t>(visibleRows > 0 ? visibleRows : 1), static_cast<uint16_t>(count)));
}

inline freeink::ui::InputSnapshot touchSnapshotFrom(const MappedInputManager& mappedInput) {
  freeink::ui::InputSnapshot snap{};
  int tx = 0;
  int ty = 0;
  if (mappedInput.wasScreenLongPress(tx, ty)) {
    snap.touchReleased = true;
    snap.longPress = true;
    snap.touchX = static_cast<int16_t>(tx);
    snap.touchY = static_cast<int16_t>(ty);
    return snap;
  }
  // Live contact position: only InputDrag-masked elements (sliders) react, so
  // carrying it in every snapshot is free for ordinary screens.
  if (mappedInput.isScreenTouchHeld(tx, ty)) {
    snap.touchHeld = true;
    snap.touchX = static_cast<int16_t>(tx);
    snap.touchY = static_cast<int16_t>(ty);
  }
  if (mappedInput.wasScreenTouchDown(tx, ty)) {
    snap.touchPressed = true;
    snap.touchX = static_cast<int16_t>(tx);
    snap.touchY = static_cast<int16_t>(ty);
  }
  if (mappedInput.wasScreenTapped(tx, ty)) {
    snap.touchReleased = true;
    snap.touchX = static_cast<int16_t>(tx);
    snap.touchY = static_cast<int16_t>(ty);
  } else if (mappedInput.wasScreenTouchReleased()) {
    snap.touchReleased = true;
    snap.touchX = -1;
    snap.touchY = -1;
  }
  // A swipe is also a cancelled row press. Some touch backends report the
  // gesture before exposing their raw release edge, so clear it explicitly
  // before list code repaints the new viewport.
  if (!snap.touchReleased && mappedInput.wasSwipe() != MappedInputManager::SwipeDir::None) {
    snap.touchReleased = true;
    snap.touchX = -1;
    snap.touchY = -1;
  }
  return snap;
}
