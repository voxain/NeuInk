#include "AppRegistry.h"

#include <cstring>

#include "CheckersActivity.h"
#include "ChessActivity.h"
#include "ClockActivity.h"
#include "GameUi.h"
#include "GoActivity.h"
#include "SudokuActivity.h"
#include "TicTacToeActivity.h"

namespace apps {
namespace {

constexpr char kSavePath[] = "/.crosspoint/games/apps.bin";
constexpr uint8_t kFileMagic[3] = {'V', 'A', 'P'};

template <typename T>
std::unique_ptr<Activity> make(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<T>(renderer, mappedInput);
}

}  // namespace

const Info kAll[kCount] = {
    {Sudoku, StrId::STR_GAME_SUDOKU, UIIcon::Sudoku, "Sudoku", make<SudokuActivity>},
    {Chess, StrId::STR_GAME_CHESS, UIIcon::Chess, "Chess", make<ChessActivity>},
    {Checkers, StrId::STR_GAME_CHECKERS, UIIcon::Checkers, "Checkers", make<CheckersActivity>},
    {Go, StrId::STR_GAME_GO, UIIcon::Go, "Go", make<GoActivity>},
    {TicTacToe, StrId::STR_GAME_TICTACTOE, UIIcon::TicTacToe, "TicTacToe", make<TicTacToeActivity>},
    {Clock, StrId::STR_APP_CLOCK, UIIcon::Clock, "Clock", make<ClockActivity>},
};

const Info* find(const Id id) {
  for (const Info& info : kAll) {
    if (info.id == id) return &info;
  }
  return nullptr;
}

const Info* findByActivityName(const std::string& activityName) {
  for (const Info& info : kAll) {
    if (activityName == info.activityName) return &info;
  }
  return nullptr;
}

uint8_t hiddenMask() {
  uint8_t bytes[4];
  return gameui::readSave(kSavePath, bytes, sizeof(bytes)) && memcmp(bytes, kFileMagic, 3) == 0 ? bytes[3] : 0;
}

bool isHidden(const Id id) { return (hiddenMask() & (1u << id)) != 0; }

void saveHiddenMask(const uint8_t mask) {
  const uint8_t bytes[4] = {kFileMagic[0], kFileMagic[1], kFileMagic[2], mask};
  gameui::writeSave(kSavePath, bytes, sizeof(bytes));
}

}  // namespace apps
