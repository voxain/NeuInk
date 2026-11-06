#pragma once

#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>

#include <atomic>

#include "activities/Activity.h"
#include "components/DrawerHandle.h"

namespace panel {

namespace fui = freeink::ui;

// The look of a panel that slides over the screen, as the quick settings one: a light-gray rounded card with a
// small grabber, white controls on it (black when picked), no outline.
constexpr int kSide = 12;          // gap between the card and the screen's side
constexpr int kRadius = 32;
constexpr int kPad = 22;           // inside the card
constexpr int kGrabberBand = 24;   // the strip a swipe or tap on closes the panel
constexpr int kControlHeight = 52;  // a switch button
constexpr int kPreviewHeight = 64;  // the picture on an option tile
constexpr int kTileHeight = 10 + kPreviewHeight + 6 + 22 + 8;  // padding, picture, name
constexpr int kLabelHeight = 26;   // the small name above a row of tiles
constexpr int kRowGap = 18;

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

// One full-width pill that is a switch: white when off, black when on.
template <size_t MaxInteractions>
void switchButton(const GfxRenderer& renderer, fui::Frame<MaxInteractions>& frame, const int x, const int y,
                  const int width, const char* label, const bool on, const fui::ActionId action, const int fontId) {
  renderer.fillRoundedRect(x, y, width, kControlHeight, kControlHeight / 2, on ? Color::Black : Color::White);
  const auto style = on ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  renderer.drawText(fontId, x + (width - renderer.getTextWidth(fontId, label, style)) / 2,
                    y + (kControlHeight - renderer.getLineHeight(fontId)) / 2, label, !on, style);
  frame.hit(fui::Rect{static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(width), kControlHeight},
            action, 0, fui::InputTouch);
}

}  // namespace panel

// A panel over the current screen, hanging from the bottom edge (the card, its grabber and the ways to close it
// are panel::card). It paints only its own band, so what is underneath stays on the screen. A subclass says how
// tall its content is and draws it with the panel:: helpers; a choice comes back through onChoice.
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
  using UiApp = freeink::ui::FreeInkApp<24, 12>;
  using Screen = UiApp::ScreenType;
  static constexpr freeink::ui::ActionId ACTION_CHOICE = 1;  // a panel::tiles / switchButton hit: value = index

  // The height of what goes inside the card, between its padding.
  virtual int contentHeight() const = 0;
  // Draw the content from y, between x and x + width, and register its hits with ACTION_CHOICE (or own actions).
  virtual void buildContent(Screen& screen, int x, int y, int width) = 0;
  // A hit on a control: which one is up to the subclass (its action id) and which option (value).
  virtual void onChoice(freeink::ui::ActionId action, int value) = 0;

 private:
  static void screenBuilder(Screen& screen, void* user);
  static void onActionEvent(const freeink::ui::ActionEvent& event, void* user);

  freeink::ui::GfxRendererTarget uiTarget;  // must precede `app`: the app holds a reference to it
  UiApp app;
  std::atomic<bool> uiReady{false};
  freeink::ui::Rect handle{};
  int cardTop = 0;
};
