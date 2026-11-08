#include "SoftwareUpdatesActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <cstdio>
#include <string>

#include "CrossPointSettings.h"
#include "SdFirmwareUpdateActivity.h"
#include "SilentRestart.h"
#include "activities/util/ConfirmationActivity.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"
#include "images/BootLogo192.h"
#include "AppVersion.h"
#include "network/OtaUpdater.h"

namespace {
constexpr int kLogo = 192;
constexpr int16_t kButtonH = 52;
}  // namespace

SoftwareUpdatesActivity::SoftwareUpdatesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : RowListActivity(renderer, mappedInput, "SoftwareUpdates", tr(STR_SOFTWARE_UPDATES)) {}

// The last check found a newer release than this one (it was not installed, or the install was cancelled).
bool SoftwareUpdatesActivity::updateWaiting() const {
  return SETTINGS.lastSeenVersion[0] != '\0' && OtaUpdater::isNewerThanInstalled(SETTINGS.lastSeenVersion);
}

void SoftwareUpdatesActivity::buildRows(std::vector<Row>& rows) {
  rows.clear();
  waiting = updateWaiting();  // read once per rebuild: the header draws from it every frame
  const auto action = [&](const char* label, std::function<void()> onTap) {
    Row row;
    row.kind = Row::Kind::Chevron;
    row.label = label;
    row.onTap = std::move(onTap);
    rows.push_back(std::move(row));
  };
  action(tr(STR_CHECK_UPDATES), [] { silentRestartToNetwork(NetworkBootTarget::OTA); });
  action(tr(STR_SWITCH_FIRMWARE), [this] { openSwitchFirmwarePicker(); });
  action(tr(STR_SD_FIRMWARE_UPDATE), [this] { openChild(std::make_unique<SdFirmwareUpdateActivity>(renderer, mappedInput)); });
}

int SoftwareUpdatesActivity::headerHeight() const {
  const int line12 = renderer.getLineHeight(UI_12_FONT_ID);
  const int line10 = renderer.getLineHeight(UI_10_FONT_ID);
  int height = kLogo + 8 + line12 + 16;
  if (SETTINGS.lastSeenVersion[0] != '\0') height += line10 * 2 + 8;
  if (waiting) height += kButtonH + 16;
  return height + 8;
}

void SoftwareUpdatesActivity::buildHeader(UiApp::ScreenType& screen, const freeink::ui::Rect& area) {
  namespace fui = freeink::ui;
  const int cx = area.x + area.width / 2;
  const auto centred = [&](const int font, const int y, const char* text, const EpdFontFamily::Style style) {
    renderer.drawText(font, cx - renderer.getTextWidth(font, text, style) / 2, y, text, true, style);
  };
  renderer.drawImage(BootLogo192, cx - kLogo / 2, area.y, kLogo, kLogo);
  int y = area.y + kLogo + 8;
  char line[64];
  snprintf(line, sizeof(line), "NeuInk %s", CROSSINK_VERSION);
  centred(UI_12_FONT_ID, y, line, EpdFontFamily::BOLD);
  y += renderer.getLineHeight(UI_12_FONT_ID) + 16;
  if (SETTINGS.lastSeenVersion[0] == '\0') return;

  // After a check: what it found, against what is installed.
  centred(UI_10_FONT_ID, y, waiting ? tr(STR_NEW_UPDATE) : tr(STR_UP_TO_DATE), EpdFontFamily::BOLD);
  y += renderer.getLineHeight(UI_10_FONT_ID);
  snprintf(line, sizeof(line), waiting ? "%s  \xE2\x86\x92  %s" : "%s  =  %s", CROSSINK_VERSION, SETTINGS.lastSeenVersion);
  centred(UI_10_FONT_ID, y, line, EpdFontFamily::REGULAR);
  y += renderer.getLineHeight(UI_10_FONT_ID) + 16;
  if (!waiting) return;

  fui::ButtonProps props;
  props.label = tr(STR_UPDATE_NOW);
  props.icon = fui::bitmapFromIcon(icon_type_download_24);
  props.action = ACTION_HEADER;
  props.value = 0;
  props.styles = grayButtonStyles();
  // The call to action is black, its label white.
  props.styles.normal.background = fui::Paint::solid(fui::Color::Black);
  props.styles.normal.foreground = fui::Paint::solid(fui::Color::White);
  props.radius = kButtonH / 2;
  props.text = screen.theme().bodyText;
  props.text.bold = true;
  props.gap = 10;
  const int16_t width = static_cast<int16_t>(area.width * 2 / 3);
  fui::button(screen.frame(), fui::Rect{static_cast<int16_t>(cx - width / 2), static_cast<int16_t>(y), width, kButtonH},
              props);
}

// "Update now": the update check again, which offers the new version to install.
void SoftwareUpdatesActivity::onHeaderAction(const int) { silentRestartToNetwork(NetworkBootTarget::OTA); }

void SoftwareUpdatesActivity::openSwitchFirmwarePicker() {
  using firmware_targets::Target;
  constexpr Target targets[] = {Target::VARAGH, Target::CROSSINK, Target::CROSSPOINT};
  std::vector<std::string> options;
  for (const Target target : targets) options.emplace_back(firmware_targets::displayName(target));

  popup.show(StrId::STR_SWITCH_FIRMWARE, options, 0, [this, targets](const int selectedIndex) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(std::size(targets))) {
      requestUpdate();
      return;
    }
    const Target target = targets[selectedIndex];
    const std::string heading = std::string(tr(STR_SWITCH_FIRMWARE_TO)) + firmware_targets::displayName(target) + "?";
    startActivityForResult(
        std::make_unique<ConfirmationActivity>(renderer, mappedInput, heading, tr(STR_SWITCH_FIRMWARE_BACKUP)),
        [this, target](const ActivityResult& result) {
          if (result.isCancelled) {
            requestUpdate();
            return;
          }
          silentRestartToNetwork(NetworkBootTarget::OTA, firmware_targets::toPayload(target));
        });
  });
  requestUpdate();
}
