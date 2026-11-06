#pragma once

#include <Icon.h>

#include <cstdint>

#include "components/icons/homeIcons.h"
#include "components/themes/BaseTheme.h"

// The Lucide icon of an app, a Home entry or a file type, at 24, 32, 42 or 48 px (see homeIcons.manifest);
// nullptr when there is none of that size.
inline const freeink::Icon* homeIcon(const UIIcon icon, const uint32_t size) {
#define HOME_ICON(ui, name)                                                                                    \
  case ui:                                                                                                     \
    return size == 24 ? &icon_##name##_24                                                                      \
                      : (size == 32 ? &icon_##name##_32                                                        \
                                    : (size == 42 ? &icon_##name##_42 : (size == 48 ? &icon_##name##_48 : nullptr)))
  switch (icon) {
    HOME_ICON(UIIcon::Folder, files);
    HOME_ICON(UIIcon::Text, type_text);
    HOME_ICON(UIIcon::Image, type_image);
    HOME_ICON(UIIcon::Book, type_book);
    HOME_ICON(UIIcon::File, type_file);
    HOME_ICON(UIIcon::Recent, type_recent);
    HOME_ICON(UIIcon::Hotspot, type_hotspot);
    HOME_ICON(UIIcon::Usb, type_usb);
    HOME_ICON(UIIcon::Download, type_download);
    HOME_ICON(UIIcon::Apps, apps_grid);
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
