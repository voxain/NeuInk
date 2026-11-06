#pragma once

#include <I18n.h>

#include <cstdint>
#include <memory>
#include <string>

#include "components/themes/BaseTheme.h"

class Activity;
class GfxRenderer;
class MappedInputManager;

// The one list of the built-in apps. The Apps screen, the Home screen of the
// themes that list the apps themselves, and sleep's "come back to the app that
// was open" all read it, so a new app is added here and nowhere else.
namespace apps {

// The numbers are kept in files (hidden apps, sleep resume): a new app gets the next
// one and none is ever reused. Slot 0 was Wordle, removed from NeuInk to save flash.
enum Id : uint8_t { Sudoku = 1, Chess = 2, Checkers = 3, Clock = 4, TicTacToe = 5, Go = 6, kIdCount = 7 };

struct Info {
  Id id;
  StrId name;
  UIIcon icon;
  const char* activityName;  // the name the app's screen gives itself (Activity::name)
  std::unique_ptr<Activity> (*create)(GfxRenderer&, MappedInputManager&);
};

// In display order: the games, then the Clock.
inline constexpr int kCount = 6;
extern const Info kAll[kCount];

const Info* find(Id id);
const Info* findByActivityName(const std::string& activityName);

// "Manage" on the Apps screen: an app that is switched off is only hidden from the
// lists, it stays in the firmware.
bool isHidden(Id id);
uint8_t hiddenMask();
void saveHiddenMask(uint8_t mask);

}  // namespace apps
