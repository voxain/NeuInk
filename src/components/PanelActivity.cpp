#include "PanelActivity.h"

#include "MappedInputManager.h"
#include "components/UiAppHelpers.h"

namespace {
constexpr freeink::ui::ActionId ACTION_DISMISS = 2;
constexpr int kControlSlots = 8;  // action ids a subclass can use from ACTION_FIRST_CONTROL
}  // namespace

PanelActivity::PanelActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const char* name)
    : Activity(name, renderer, mappedInput), uiTarget(makeUiTarget(renderer)), app(uiTarget, uiTarget.deviceContext()) {}

void PanelActivity::onEnter() {
  Activity::onEnter();
  uiReady = false;
  applySharedUiTheme(app, uiTarget);
  app.on(ACTION_DISMISS, &PanelActivity::onActionEvent, this);
  for (int i = 0; i < kControlSlots; ++i) app.on(ACTION_FIRST_CONTROL + i, &PanelActivity::onActionEvent, this);
  app.setScreen(&PanelActivity::screenBuilder, this);
  requestUpdate();
}

void PanelActivity::onActionEvent(const freeink::ui::ActionEvent& event, void* user) {
  auto* self = static_cast<PanelActivity*>(user);
  if (event.action == ACTION_DISMISS) {
    self->finish();
  } else {
    self->onChoice(event.action, event.value);
  }
}

void PanelActivity::screenBuilder(Screen& screen, void* user) {
  auto* self = static_cast<PanelActivity*>(user);
  const int bottom = self->renderer.getScreenHeight() - panel::kSide;
  self->handle = panel::card(self->renderer, screen.frame(), freeink::ui::SheetEdge::Bottom, self->cardTop, bottom,
                             ACTION_DISMISS);
  const int x = panel::kSide + panel::kPad;
  self->buildContent(screen, x, self->cardTop + panel::kGrabberBand + panel::kPad / 2,
                     self->renderer.getScreenWidth() - 2 * x);
}

void PanelActivity::loop() {
  if (uiReady) {
    const auto snap = touchSnapshotFrom(mappedInput);
    if (snap.touchPressed || snap.touchHeld || snap.touchReleased) {
      app.route(snap);
      if (app.invalidated()) requestUpdate();
      if (snap.touchReleased) return;
    }
  }
  if (DrawerHandle::wasDismissSwipe(mappedInput, handle, freeink::ui::SheetEdge::Bottom) ||
      mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
  }
}

void PanelActivity::render(RenderLock&&) {
  // Over the screen we opened on: only the card's band is repainted, white behind it.
  const int height = panel::kGrabberBand + panel::kPad / 2 + contentHeight() + panel::kPad;
  cardTop = renderer.getScreenHeight() - panel::kSide - height;
  uiReady = false;
  renderer.fillRect(0, cardTop - 6, renderer.getScreenWidth(), renderer.getScreenHeight() - cardTop + 6, false);
  renderer.setSmoothRoundEnds(true);
  app.render();
  renderer.setSmoothRoundEnds(false);
  uiReady = true;
  renderer.displayBuffer();
}
