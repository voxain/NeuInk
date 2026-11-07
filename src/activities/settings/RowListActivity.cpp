#include "RowListActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"

namespace fui = freeink::ui;

namespace {
constexpr fui::ActionId ACTION_ROW = 1;
}

RowListActivity::RowListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string name,
                                 std::string title)
    : Activity(std::move(name), renderer, mappedInput), title(std::move(title)), ui(renderer) {}

void RowListActivity::onEnter() {
  Activity::onEnter();
  buildRows(rows);
  selectedIndex = 0;
  while (selectedIndex < static_cast<int>(rows.size()) && !selectable(selectedIndex)) ++selectedIndex;
  ui.closeRouting();
  visibleRows = 1;
  topIndex = 0;
  listNav.reset(selectedIndex);
  ui.reset();
  ui.app.on(ACTION_ROW, &RowListActivity::onRowEvent, this);
  ui.app.setScreen(&RowListActivity::listScreen, this);
  requestUpdate();
}

bool RowListActivity::selectable(const int index) const {
  return index >= 0 && index < static_cast<int>(rows.size()) && rows[index].kind != Row::Kind::Header;
}

void RowListActivity::refresh() {
  {
    RenderLock lock(*this);
    buildRows(rows);
    if (selectedIndex >= static_cast<int>(rows.size())) selectedIndex = static_cast<int>(rows.size()) - 1;
  }
  requestUpdate();
}

void RowListActivity::activate(const int index) {
  if (!selectable(index) || !rows[index].onTap) return;
  // Copy: the tap may rebuild the rows it came from.
  const auto onTap = rows[index].onTap;
  onTap();
  refresh();
}

void RowListActivity::select(const int index) {
  RenderLock lock(*this);
  selectedIndex = index;
  listNav.selected = index;
  listNav.top = topIndex;
  listNav.visibleRows = visibleRows;
  listNav.follow(static_cast<int>(rows.size()));
  topIndex = listNav.top;
}

void RowListActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<RowListActivity*>(user);
  self->ui.app.clearTapFlash();
  self->activate(event.value);
}

void RowListActivity::loop() {
  if (popup.handleInput(mappedInput, [this] { requestUpdate(); })) return;
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    setResult(ActivityResult{});
    finish();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    activate(selectedIndex);
    return;
  }
  if (ui.routingReady()) {
    fui::ActionEvent event{};
    if (ui.routeTouch(mappedInput, event)) {
      if (ui.app.invalidated()) requestUpdate();
      if (event) return;
    }
  }

  const int count = static_cast<int>(rows.size());
  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Down) {
    bool moved;
    {
      RenderLock lock(*this);
      listNav.selected = selectedIndex;
      listNav.top = topIndex;
      listNav.visibleRows = visibleRows;
      const int page = listNav.pageRowsFor(count);
      moved = listNav.scrollBy(swipe == MappedInputManager::SwipeDir::Up ? page : -page, count);
      topIndex = listNav.top;
    }
    if (moved) requestUpdate();
    return;
  }

  const auto step = [this, count](const int direction) {
    int next = selectedIndex;
    for (int tries = 0; tries < count; ++tries) {
      next = direction > 0 ? ButtonNavigator::nextIndex(next, count) : ButtonNavigator::previousIndex(next, count);
      if (selectable(next)) break;
    }
    select(next);
    requestUpdate();
  };
  buttonNavigator.onNext([&step] { step(1); });
  buttonNavigator.onPrevious([&step] { step(-1); });
}

void RowListActivity::listScreen(UiApp::ScreenType& screen, void* user) {
  static_cast<RowListActivity*>(user)->buildListScreen(screen);
}

void RowListActivity::buildListScreen(UiApp::ScreenType& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMargin(
      fui::Insets{static_cast<int16_t>(metrics.topPadding + TouchHeaderBackButton::height(metrics, mappedInput)), 0,
                  static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  bool anySubtitle = false;
  std::vector<fui::ListItem> items;
  items.reserve(rows.size());
  for (size_t i = 0; i < rows.size(); ++i) {
    const Row& row = rows[i];
    fui::ListItem item;
    item.label = row.label.c_str();
    if (row.icon >= 0) item.icon = listIconFor(static_cast<UIIcon>(row.icon));
    if (!row.subtitle.empty()) {
      item.subtitle = row.subtitle.c_str();
      anySubtitle = true;
    }
    item.isHeader = row.kind == Row::Kind::Header;
    item.toggle = row.kind == Row::Kind::Toggle;
    item.toggleChecked = row.checked;
    if (row.kind == Row::Kind::Chevron) {
      item.value = ">";
    } else if (row.kind == Row::Kind::Value && !row.value.empty()) {
      item.value = row.value.c_str();
    }
    item.actionValue = static_cast<int16_t>(i);
    items.push_back(item);
  }

  fui::ListProps props;
  props.items = items.data();
  props.count = static_cast<uint16_t>(items.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;  // physical buttons stay in loop()
  props.selectedIndex = static_cast<int16_t>(selectedIndex);
  props.labelText = screen.theme().bodyText;
  props.labelText.maxLines = 2;
  props.subtitleText = screen.theme().smallText;
  props.subtitleText.maxLines = 3;
  props.headerText = screen.theme().bodyText;
  props.headerText.bold = true;
  const auto visible = configureUiList(props, screen.theme(), screen.body(),
                                       anySubtitle ? UiListRowType::WithSubtitle : UiListRowType::SingleLine);
  visibleRows = visible > 0 ? visible : 1;
  listNav.selected = selectedIndex;
  listNav.top = topIndex;
  listNav.visibleRows = visibleRows;
  listNav.syncToProps(screen.body(), props.rowHeight, props.rowGap, static_cast<int>(items.size()), props);
  topIndex = listNav.top;
  screen.list(props);
  topIndex = listNav.top;
}

void RowListActivity::render(RenderLock&&) {
  if (popup.processRender(renderer, mappedInput)) return;
  renderer.clearScreen();
  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::draw(renderer, ui.target, header, title.c_str(), false);
  } else {
    GUI.drawHeader(renderer, header, title.c_str());
  }

  ui.closeRouting();
  for (int pass = 0; pass < 8; ++pass) {
    ui.render();
    if (!listNav.consumeRebuildNeeded()) break;
  }

  const auto labels =
      mappedInput.mapLabels(mappedInput.withBackArrow(tr(STR_BACK)), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
