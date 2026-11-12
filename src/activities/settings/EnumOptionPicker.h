#pragma once

#include <I18n.h>

#include <functional>
#include <string>
#include <vector>

#include "CrossPointSettings.h"
#include "QuickActions.h"
#include "SettingsList.h"
#include "activities/settings/SettingsActivity.h"
#include "components/OptionPopup.h"

// True when an enum setting is edited through the option popup rather than cycled in place.
inline bool enumSettingUsesOptionMenu(const SettingInfo& setting) {
  return setting.nameId != StrId::STR_FONT_FAMILY && setting.type == SettingType::ENUM &&
         settingEnumOptionCount(setting) > 2 &&
         (setting.valuePtr != nullptr || (setting.valueGetter && setting.valueSetter));
}

// Stores the option picked at `displayIndex` for an enum setting, through its member pointer or its setter.
inline void applyEnumSelection(const SettingInfo& setting, const int displayIndex) {
  const auto index = static_cast<uint8_t>(displayIndex);
  if (setting.valuePtr != nullptr) {
    SETTINGS.*(setting.valuePtr) = enumRawValueForDisplayIndex(setting, index);
    if (isTwoFingerSwipeSetting(setting.valuePtr)) {
      CrossPointSettings::normalizeTwoFingerSwipeActions(SETTINGS, setting.valuePtr);
    }
    QuickActions::settingChanged(SETTINGS, setting.valuePtr);
  } else if (setting.valueSetter) {
    setting.valueSetter(index);
  }
}

// Opens `popup` listing the options of an enum setting. The pick is applied first, then `onChosen` runs
// (persist, refresh and so on).
inline void showEnumOptionPicker(OptionPopup& popup, const bool hasTouchHardware, const SettingInfo& setting,
                                 std::function<void()> onChosen) {
  const size_t optionCount = settingEnumOptionCount(setting);
  if (optionCount == 0) return;

  std::vector<std::string> options;
  options.reserve(optionCount);
  for (size_t i = 0; i < optionCount; i++) {
    options.push_back(sideButtonOptionLabel(setting, static_cast<uint8_t>(i)));
  }

  uint8_t currentIndex = 0;
  if (setting.valuePtr != nullptr) {
    currentIndex = enumDisplayIndexForRawValue(setting, SETTINGS.*(setting.valuePtr));
  } else if (setting.valueGetter) {
    currentIndex = setting.valueGetter();
  }
  if (currentIndex >= optionCount) currentIndex = 0;

  const bool showEscapeHatchNote = setting.valuePtr == &CrossPointSettings::sideButtonChordAction && hasTouchHardware;
  popup.show(
      setting.nameId, options, currentIndex,
      [setting, onChosen = std::move(onChosen)](const int selectedIndex) {
        applyEnumSelection(setting, selectedIndex);
        onChosen();
      },
      showEscapeHatchNote ? OptionPopup::Note{tr(STR_NOTE), tr(STR_TOUCHSCREEN_ESCAPE_HATCH_NOTE)}
                          : OptionPopup::Note{});
}
