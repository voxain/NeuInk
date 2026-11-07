#include "VegaTheme.h"

#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalGPIO.h>
#include <ScalableBuiltins.h>
#include <HalPowerManager.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>

#include "BuiltinBitter.h"
#include "activities/Activity.h"
#include "activities/apps/AppRegistry.h"
#include "components/LevelBox.h"
#include "components/UITheme.h"
#include "CrossPointSettings.h"
#include "components/UiAppHelpers.h"
#include "components/icons/homeIconSet.h"
#include "fontIds.h"
#include "network/OnlineWifi.h"
#include "util/FrontlightSchedule.h"
#include "util/LocalClockOffset.h"

namespace {
constexpr int kHeaderIconSize = 24;
constexpr int kGreetingGap = 10;  // above and below the greeting text

// The greeting is set large, in Bitter. Scalable builds (X4 Pro) load the Bitter family on demand, the same one
// the reader uses when Bitter is its font, and the largest scalable size is 22. Bitmap builds use their largest
// built-in size (16), and LexendDeca where Bitter is not built in.
int greetingFontId(GfxRenderer& renderer) {
#if CROSSINK_SCALABLE_FONTS
  const unsigned bitter = CrossPointSettings::BITTER;
  ensureScalableBuiltinFamily(renderer, bitter);
  return scalableBuiltinReaderFontId(bitter, 22);
#elif CROSSINK_BUILTIN_BITTER
  (void)renderer;
  return BITTER_16_FONT_ID;
#else
  (void)renderer;
  return LEXENDDECA_16_FONT_ID;
#endif
}

// "Good morning", "Good evening"... by the local hour, in the menu language.
const char* greetingText() {
  uint8_t utcHour = 0;
  uint8_t utcMinute = 0;
  if (!halClock.getTime(utcHour, utcMinute)) return tr(STR_GREETING_HELLO);
  const int hour = FrontlightSchedule::localTimeOfDay(utcHour, utcMinute, localClockOffsetQ()) / 60;
  if (hour >= 5 && hour < 12) return tr(STR_GREETING_MORNING);
  if (hour >= 12 && hour < 18) return tr(STR_GREETING_AFTERNOON);
  if (hour >= 18 && hour < 22) return tr(STR_GREETING_EVENING);
  return tr(STR_GREETING_NIGHT);
}

}  // namespace

int VegaTheme::homeGreetingHeight(GfxRenderer& renderer) const {
  if (!SETTINGS.homeGreeting) return 0;
  return renderer.getLineHeight(greetingFontId(renderer)) + 2 * kGreetingGap;
}

void VegaTheme::drawGreeting(GfxRenderer& renderer, Rect rect) const {
  if (!SETTINGS.homeGreeting) return;
  const int fontId = greetingFontId(renderer);
  const int y = rect.y + VegaMetrics::values.topPadding + kGreetingGap;
  renderer.drawText(fontId, VegaMetrics::values.contentSidePadding, y, greetingText(), true, EpdFontFamily::BOLD);
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
      {"SavedItemsHome", UIIcon::BookmarkIcon}, {"Battery", UIIcon::Battery},   {"Storage", UIIcon::StorageIcon},
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
  const int centerY = rect.y + rect.height / 2;  // the middle of the status bar, where the clock is
  const int bodyX = batteryLeft ? rect.x : rect.x + rect.width - nubW - bodyW;

  // The empty part is light gray, or white where the bar itself is that gray.
  levelbox::battery(renderer, bodyX, centerY, bodyW, bodyH, 8, percentage,
                    showPercentage ? std::to_string(percentage).c_str() : nullptr, SMALL_FONT_ID,
                    onBand ? Color::White : Color::LightGray);

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
