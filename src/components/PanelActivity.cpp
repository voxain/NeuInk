#include "PanelActivity.h"

#include <algorithm>

#include "MappedInputManager.h"
#include "components/UiAppHelpers.h"

namespace {
constexpr freeink::ui::ActionId ACTION_DISMISS = 2;
constexpr freeink::ui::ActionId ACTION_CORNER = 3;
constexpr int kControlSlots = 8;  // action ids a subclass can use from ACTION_FIRST_CONTROL
constexpr int kCornerButton = 44;
constexpr int kTopInset = panel::kGrabberBand + panel::kPad / 2;  // the card's top to its content
}  // namespace

// The card's top to its content: below the corner button when there is one.
int PanelActivity::topInset() const {
  if (cornerIcon() != nullptr) return 10 + kCornerButton + 6;
  return anchor() == freeink::ui::SheetEdge::Top ? panel::kPad : kTopInset;
}

namespace {
constexpr int kTopCardTop = 34;  // a card hanging from the top starts below the status row, as the quick controls
}  // namespace

PanelActivity::PanelActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const char* name)
    : Activity(name, renderer, mappedInput), uiTarget(makeUiTarget(renderer)), app(uiTarget, uiTarget.deviceContext()) {}

void PanelActivity::onEnter() {
  Activity::onEnter();
  uiReady = false;
  firstSection = 0;
  paintedTop = renderer.getScreenHeight();
  paintedBottom = 0;
  applySharedUiTheme(app, uiTarget);
  app.on(ACTION_DISMISS, &PanelActivity::onActionEvent, this);
  app.on(ACTION_CORNER, &PanelActivity::onActionEvent, this);
  for (int i = 0; i < kControlSlots; ++i) app.on(ACTION_FIRST_CONTROL + i, &PanelActivity::onActionEvent, this);
  app.setScreen(&PanelActivity::screenBuilder, this);
  requestUpdate();
}

int PanelActivity::maxContentHeight() const {
  return renderer.getScreenHeight() - 2 * panel::kSide - topInset() - panel::kPad - 60;
}

int PanelActivity::visibleHeight() const {
  int total = 0;
  for (int i = 0; i < sectionCount(); ++i) total += sectionHeight(i);
  return std::min(total, maxContentHeight());
}

int PanelActivity::lastVisible(const int first) const {
  const int room = visibleHeight();
  int used = 0;
  int i = first;
  // The first section always shows, even when it alone is taller than the room.
  while (i < sectionCount() && (i == first || used + sectionHeight(i) <= room)) used += sectionHeight(i++);
  return i;
}

void PanelActivity::scrollPage(const bool down) {
  int next = firstSection;
  if (down) {
    next = lastVisible(firstSection);
    if (next >= sectionCount()) return;
  } else {
    // Back by as many sections as fit above.
    int used = 0;
    while (next > 0 && used + sectionHeight(next - 1) <= visibleHeight()) used += sectionHeight(--next);
    if (next == firstSection && next > 0) --next;
  }
  if (next == firstSection) return;
  firstSection = next;
  requestUpdate();
}

void PanelActivity::onActionEvent(const freeink::ui::ActionEvent& event, void* user) {
  auto* self = static_cast<PanelActivity*>(user);
  if (event.action == ACTION_DISMISS) {
    self->finish();
  } else if (event.action == ACTION_CORNER) {
    self->onCorner();
  } else {
    self->onChoice(event.action, event.value);
  }
}

void PanelActivity::screenBuilder(Screen& screen, void* user) {
  auto* self = static_cast<PanelActivity*>(user);
  const GfxRenderer& renderer = self->renderer;
  const int width = renderer.getScreenWidth();
  auto& frame = screen.frame();
  self->handle = panel::card(renderer, frame, self->anchor(), self->cardTop, self->cardBottom, ACTION_DISMISS);
  const int x = panel::kSide + panel::kPad;
  const int contentW = width - 2 * x;
  const int top = self->cardTop + self->topInset();
  if (self->firstSection >= self->sectionCount()) self->firstSection = 0;
  const int end = self->lastVisible(self->firstSection);
  int y = top;
  for (int i = self->firstSection; i < end; ++i) {
    self->buildSection(screen, i, x, y, contentW);
    y += self->sectionHeight(i);
  }
  // More above or below: a thin scroll bar on the right edge of the card.
  if (self->firstSection > 0 || end < self->sectionCount()) {
    const int trackH = self->visibleHeight();
    const int count = self->sectionCount();
    const int thumbY = top + trackH * self->firstSection / count;
    const int thumbH = std::max(24, trackH * (end - self->firstSection) / count);
    const int barX = width - panel::kSide - 9;
    renderer.fillRoundedRect(barX, top, 4, trackH, 2, Color::White);
    renderer.fillRoundedRect(barX, std::min(thumbY, top + trackH - thumbH), 4, thumbH, 2, Color::Black);
  }
  if (const freeink::Icon* icon = self->cornerIcon()) {
    panel::stepButton(renderer, frame, width - panel::kSide - panel::kPad / 2 - kCornerButton, self->cardTop + 10,
                      kCornerButton, *icon, ACTION_CORNER, 0, 8, 8);
  }
}

void PanelActivity::loop() {
  if (uiReady) {
    const auto snap = touchSnapshotFrom(mappedInput);
    if (snap.touchPressed || snap.touchHeld || snap.touchReleased) {
      app.route(snap);
      if (app.invalidated()) requestUpdate();
    }
  }
  // A swipe up or down on the content turns its page; one on the grabber closes the panel.
  MappedInputManager::SwipeDir direction = MappedInputManager::SwipeDir::None;
  int startX = 0;
  int startY = 0;
  int endX = 0;
  int endY = 0;
  if (mappedInput.wasSwipeWithPoints(direction, startX, startY, endX, endY) &&
      (direction == MappedInputManager::SwipeDir::Up || direction == MappedInputManager::SwipeDir::Down) &&
      startY >= cardTop && startY < cardBottom && !handle.contains(static_cast<int16_t>(startX), static_cast<int16_t>(startY))) {
    scrollPage(direction == MappedInputManager::SwipeDir::Up);
    return;
  }
  if (DrawerHandle::wasDismissSwipe(mappedInput, handle, anchor()) ||
      mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
  }
}

void PanelActivity::render(RenderLock&&) {
  const bool atTop = anchor() == freeink::ui::SheetEdge::Top;
  const int height = topInset() + visibleHeight() + (atTop ? panel::kPad / 2 + panel::kGrabberBand : panel::kPad);
  const int screenH = renderer.getScreenHeight();
  cardTop = atTop ? kTopCardTop : screenH - panel::kSide - height;
  cardBottom = cardTop + height;
  uiReady = false;
  if (backdrop) {
    backdrop();
  } else {
    // Over the screen we opened on: only the card's band is repainted, white behind it. A card that got shorter
    // leaves its old edge behind: clear to the farther of the two.
    const int clearTop = atTop ? 0 : std::min(cardTop, paintedTop) - 6;
    const int clearBottom = atTop ? std::max(cardBottom, paintedBottom) + 6 : screenH;
    renderer.fillRect(0, clearTop, renderer.getScreenWidth(), clearBottom - clearTop, false);
  }
  paintedTop = cardTop;
  paintedBottom = cardBottom;
  renderer.setSmoothRoundEnds(true);
  app.render();
  renderer.setSmoothRoundEnds(false);
  uiReady = true;
  renderer.displayBuffer();
}
