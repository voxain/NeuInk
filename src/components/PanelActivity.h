#pragma once

#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>
#include <FreeInkUIIcon.h>

#include <Icon.h>

#include <atomic>

#include "activities/Activity.h"
#include "components/DrawerHandle.h"
#include "components/LevelBox.h"
#include "components/icons/homeIcons.h"

namespace panel {

namespace fui = freeink::ui;

// The look of a panel that slides over the screen, as the quick settings one: a light-gray rounded card with a
// small grabber, white controls on it (black when picked), no outline.
constexpr int kSide = 12;          // gap between the card and the screen's side
constexpr int kRadius = 32;
constexpr int kPad = 22;           // inside the card
constexpr int kGrabberBand = 24;   // the strip a swipe or tap on closes the panel
constexpr int kControlHeight = 52;  // a toggle row
constexpr int kPreviewHeight = 64;  // the picture on an option tile
constexpr int kTileHeight = 10 + kPreviewHeight + 6 + 22 + 8;  // padding, picture, name
constexpr int kLabelHeight = 26;   // the small name above a row of tiles
constexpr int kRowGap = 18;

// White controls on the card, black (white label) when picked, focused or pressed.
inline fui::StyleSet whiteStyles(const uint8_t radius) {
  fui::StyleSet styles;
  styles.explicitlySet = true;
  styles.normal.background = fui::Paint::solid(fui::Color::White);
  styles.normal.radius = radius;
  styles.selected = styles.normal;
  styles.selected.background = fui::Paint::solid(fui::Color::Black);
  styles.selected.foreground = fui::Paint::solid(fui::Color::White);
  styles.focused = styles.selected;
  styles.active = styles.selected;
  styles.disabled = styles.normal;
  return styles;
}

// A level slider as the quick settings' brightness: a white box filled black to `permille` with `caption` in it,
// which a finger drags (the action's event carries the position as dragPermille). `hitPad` makes the touch area
// taller than the box, above and below.
template <size_t MaxInteractions>
void levelSlider(const GfxRenderer& renderer, fui::Frame<MaxInteractions>& frame, const int x, const int y,
                 const int width, const int height, const int radius, const int permille, const char* caption,
                 const int fontId, const fui::ActionId action, const int hitPad = 0) {
  levelbox::draw(renderer, x, y, width, height, radius, width * permille / 1000, caption, fontId, Color::White);
  frame.hit(fui::Rect{static_cast<int16_t>(x), static_cast<int16_t>(y - hitPad), static_cast<int16_t>(width),
                      static_cast<int16_t>(height + 2 * hitPad)},
            action, 0, fui::InputTouch | fui::InputDrag);
}

// A round button (white, or `color`) with a Lucide icon, for exact steps beside a slider. `padX` and `padY` widen its
// touch area.
template <size_t MaxInteractions>
void stepButton(const GfxRenderer& renderer, fui::Frame<MaxInteractions>& frame, const int x, const int y,
                const int diameter, const freeink::Icon& icon, const fui::ActionId action, const int value,
                const int padX, const int padY, const Color color = Color::White) {
  renderer.fillRoundedRect(x, y, diameter, diameter, diameter / 2, color);
  frame.target().bitmap(fui::Rect{static_cast<int16_t>(x + (diameter - icon.w) / 2),
                                  static_cast<int16_t>(y + (diameter - icon.h) / 2), static_cast<int16_t>(icon.w),
                                  static_cast<int16_t>(icon.h)},
                        fui::bitmapFromIcon(icon), fui::BitmapMode::Center);
  frame.hit(fui::Rect{static_cast<int16_t>(x - padX), static_cast<int16_t>(y - padY),
                      static_cast<int16_t>(diameter + 2 * padX), static_cast<int16_t>(diameter + 2 * padY)},
            action, static_cast<int16_t>(value), fui::InputTouch);
}

// The card's surface and the ways to close it: a tap outside, a tap or swipe on the grabber. `anchor` is the edge
// the card hangs from. Returns the grabber's hit rectangle (for the swipe).
template <size_t MaxInteractions>
fui::Rect card(const GfxRenderer& renderer, fui::Frame<MaxInteractions>& frame, const fui::SheetEdge anchor,
               const int top, const int bottom, const fui::ActionId dismiss) {
  const int width = renderer.getScreenWidth();
  renderer.fillRoundedRect(kSide, top, width - 2 * kSide, bottom - top, kRadius, Color::LightGray);
  const bool atTop = anchor == fui::SheetEdge::Top;
  const fui::Rect outside = atTop ? fui::Rect{0, static_cast<int16_t>(bottom + 6), static_cast<int16_t>(width),
                                              static_cast<int16_t>(renderer.getScreenHeight() - bottom - 6)}
                                  : fui::Rect{0, 0, static_cast<int16_t>(width), static_cast<int16_t>(top - 6)};
  frame.hit(outside, dismiss, 0, fui::InputTouch);
  const fui::Rect handle{static_cast<int16_t>(width / 2 - 60), static_cast<int16_t>(atTop ? bottom - kGrabberBand : top),
                         120, kGrabberBand};
  frame.hit(handle, dismiss, 0, fui::InputTouch);
  renderer.fillRoundedRect(width / 2 - 24, atTop ? bottom - 14 : top + 8, 48, 6, 3, Color::White);
  return handle;
}

// A row of option tiles side by side, the picked one black and the others white. `preview(cx, top, on, i)` draws
// option i's picture centred on cx from y = top (kPreviewHeight tall) and `label(i)` names it underneath. Each hit
// carries its index.
template <size_t MaxInteractions, typename Preview, typename Label>
void tiles(const GfxRenderer& renderer, fui::Frame<MaxInteractions>& frame, const int x, const int y,
           const int width, const int count, const int selected, const fui::ActionId action, const int fontId,
           Preview preview, Label label) {
  const int gap = 8;
  const int each = (width - (count - 1) * gap) / count;
  for (int i = 0; i < count; ++i) {
    const int bx = x + i * (each + gap);
    const bool on = i == selected;
    renderer.fillRoundedRect(bx, y, each, kTileHeight, 20, on ? Color::Black : Color::White);
    preview(bx + each / 2, y + 10, on, i);
    const char* name = label(i);
    const auto style = on ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
    renderer.drawText(fontId, bx + (each - renderer.getTextWidth(fontId, name, style)) / 2,
                      y + 10 + kPreviewHeight + 6, name, !on, style);
    frame.hit(fui::Rect{static_cast<int16_t>(bx), static_cast<int16_t>(y), static_cast<int16_t>(each), kTileHeight},
              action, static_cast<int16_t>(i), fui::InputTouch);
  }
}

// A white row with its name and an on/off toggle (the SDK's own) at the right edge; a tap anywhere on the row flips it.
template <size_t MaxInteractions>
void toggleRow(const GfxRenderer& renderer, fui::Frame<MaxInteractions>& frame, const int x, const int y,
               const int width, const char* label, const bool on, const fui::ActionId action, const int fontId,
               const int value = 0) {
  renderer.fillRoundedRect(x, y, width, kControlHeight, kControlHeight / 2, Color::White);
  renderer.drawText(fontId, x + 22, y + (kControlHeight - renderer.getLineHeight(fontId)) / 2, label, true);
  fui::ToggleProps toggle;
  toggle.checked = on;
  toggle.width = 56;
  toggle.height = 32;
  toggle.knobInset = 4;
  fui::toggle(frame, fui::Rect{static_cast<int16_t>(x + width - 22 - toggle.width),
                               static_cast<int16_t>(y + (kControlHeight - toggle.height) / 2), toggle.width,
                               toggle.height},
              toggle);
  frame.hit(fui::Rect{static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(width), kControlHeight},
            action, static_cast<int16_t>(value), fui::InputTouch);
}

// A white row with its name and a number between a minus and a plus button; a hit carries -1 or +1.
template <size_t MaxInteractions>
void stepperRow(const GfxRenderer& renderer, fui::Frame<MaxInteractions>& frame, const int x, const int y,
                const int width, const char* label, const char* value, const fui::ActionId action, const int fontId) {
  constexpr int diameter = 38;
  constexpr int valueWidth = 64;
  renderer.fillRoundedRect(x, y, width, kControlHeight, kControlHeight / 2, Color::White);
  const int textY = y + (kControlHeight - renderer.getLineHeight(fontId)) / 2;
  renderer.drawText(fontId, x + 22, textY, label, true);
  const int buttonY = y + (kControlHeight - diameter) / 2;
  const int plusX = x + width - 8 - diameter;
  const int minusX = plusX - valueWidth - diameter;
  stepButton(renderer, frame, minusX, buttonY, diameter, icon_cc_minus_24, action, -1, 6, 8, Color::LightGray);
  stepButton(renderer, frame, plusX, buttonY, diameter, icon_cc_plus_24, action, 1, 6, 8, Color::LightGray);
  renderer.drawText(fontId, minusX + diameter + (valueWidth - renderer.getTextWidth(fontId, value, EpdFontFamily::BOLD)) / 2,
                    textY, value, true, EpdFontFamily::BOLD);
}

// A row's name in small bold, above the tiles or rows it names.
inline void heading(const GfxRenderer& renderer, const int x, const int y, const char* text, const int fontId) {
  renderer.drawText(fontId, x, y, text, true, EpdFontFamily::BOLD);
}

}  // namespace panel

// A panel over the current screen, hanging from the bottom edge (the card, its grabber and the ways to close it
// are panel::card). It paints only its own band, so what is underneath stays on the screen. A subclass's content
// comes in sections drawn with the panel:: helpers; a card taller than maxContentHeight() shows as many whole
// sections as fit and scrolls a page of them with a swipe up or down on it. A choice comes back through onChoice.
class PanelActivity : public Activity {
 public:
  PanelActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const char* name);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool requiresFreshBackdrop() const override { return true; }

  // A subclass's controls register under ids from this one up (eight of them).
  static constexpr freeink::ui::ActionId ACTION_FIRST_CONTROL = 10;

 protected:
  using UiApp = freeink::ui::FreeInkApp<32, 12>;
  using Screen = UiApp::ScreenType;

  virtual int sectionCount() const = 0;
  // A section's height, its gap to the next one included.
  virtual int sectionHeight(int section) const = 0;
  // Draw a section from y, between x and x + width, and register its hits under ACTION_FIRST_CONTROL and up.
  virtual void buildSection(Screen& screen, int section, int x, int y, int width) = 0;
  // A hit on a control: which one is up to the subclass (its action id) and which option (value).
  virtual void onChoice(freeink::ui::ActionId action, int value) = 0;
  // The tallest the content may be before it scrolls (default: nearly the whole screen).
  virtual int maxContentHeight() const;
  // A round button in the card's top right corner (nullptr: none) and its tap.
  virtual const freeink::Icon* cornerIcon() const { return nullptr; }
  virtual void onCorner() {}
  // The card's top to its content (below the corner button when there is one).
  int topInset() const;
  // After the content changed its sections: back to the first one.
  void scrollToTop() { firstSection = 0; }

 private:
  static void screenBuilder(Screen& screen, void* user);
  static void onActionEvent(const freeink::ui::ActionEvent& event, void* user);
  int visibleHeight() const;
  int lastVisible(int first) const;  // one past the last section that fits from `first`
  void scrollPage(bool down);

  freeink::ui::GfxRendererTarget uiTarget;  // must precede `app`: the app holds a reference to it
  UiApp app;
  std::atomic<bool> uiReady{false};
  freeink::ui::Rect handle{};
  int cardTop = 0;
  int firstSection = 0;
  int paintedTop = 0;
};
