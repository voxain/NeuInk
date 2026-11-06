#pragma once

#include <Icon.h>

#include <cstdint>

#include "components/icons/homeIcons.h"
#include "components/themes/BaseTheme.h"

// The Lucide icon of an app or Home entry, at 24 or 32 px (see homeIcons.manifest);
// nullptr when there is none of that size.
inline const freeink::Icon* homeIcon(const UIIcon icon, const uint32_t size) {
#define HOME_ICON(ui, name)                                       \
  case ui:                                                        \
    return size == 24 ? &icon_##name##_24 : (size == 32 ? &icon_##name##_32 : nullptr)
  switch (icon) {
    HOME_ICON(UIIcon::Folder, files);
    HOME_ICON(UIIcon::Library, library);
    HOME_ICON(UIIcon::Opds, opds);
    HOME_ICON(UIIcon::Chart, stats);
    HOME_ICON(UIIcon::Settings, settings);
    HOME_ICON(UIIcon::Transfer, transfer_arrows);
    HOME_ICON(UIIcon::BookmarkIcon, highlights);
    HOME_ICON(UIIcon::Wifi, wifi_on);
    HOME_ICON(UIIcon::Sudoku, sudoku);
    HOME_ICON(UIIcon::Chess, chess);
    HOME_ICON(UIIcon::Checkers, checkers);
    HOME_ICON(UIIcon::Go, go);
    HOME_ICON(UIIcon::TicTacToe, tictactoe);
    HOME_ICON(UIIcon::Clock, clock);
    default:
      return nullptr;
  }
#undef HOME_ICON
}
