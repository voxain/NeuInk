#pragma once

#include <Icon.h>

#include <cstdint>

#include "components/icons/homeGridIcons.h"
#include "components/icons/homeIcons.h"
#include "components/themes/BaseTheme.h"

// The Lucide icon of an app, a Home entry or a file type, at 24 or 32 px, the Home grid's also at 42 and 48 (see homeIcons.manifest, homeGridIcons.manifest);
// nullptr when there is none of that size.
inline const freeink::Icon* homeIcon(const UIIcon icon, const uint32_t size) {
#define HOME_ICON(ui, name)                                               \
  case ui:                                                                \
    return size == 24 ? &icon_##name##_24 : (size == 32 ? &icon_##name##_32 : nullptr)
  // The Home grid's icons also come in the larger sizes the Home layout panel offers.
#define HOME_GRID_ICON(ui, name)                                                                                \
  case ui:                                                                                                      \
    return size == 24 ? &icon_##name##_24                                                                       \
                      : (size == 32 ? &icon_##name##_32                                                         \
                                    : (size == 42 ? &icon_##name##_42 : (size == 48 ? &icon_##name##_48 : nullptr)))
  switch (icon) {
    HOME_GRID_ICON(UIIcon::Folder, files);
    HOME_ICON(UIIcon::Text, type_text);
    HOME_ICON(UIIcon::Image, type_image);
    HOME_GRID_ICON(UIIcon::Book, type_book);
    HOME_ICON(UIIcon::File, type_file);
    HOME_ICON(UIIcon::Recent, type_recent);
    HOME_ICON(UIIcon::Hotspot, type_hotspot);
    HOME_ICON(UIIcon::Usb, type_usb);
    HOME_ICON(UIIcon::Download, type_download);
    HOME_GRID_ICON(UIIcon::Apps, apps_grid);
    HOME_GRID_ICON(UIIcon::Library, library);
    HOME_GRID_ICON(UIIcon::Opds, opds);
    HOME_GRID_ICON(UIIcon::Chart, stats);
    HOME_GRID_ICON(UIIcon::Settings, settings);
    HOME_ICON(UIIcon::Transfer, transfer_arrows);
    HOME_GRID_ICON(UIIcon::BookmarkIcon, highlights);
    HOME_ICON(UIIcon::Wifi, wifi_on);
    HOME_GRID_ICON(UIIcon::Sudoku, sudoku);
    HOME_GRID_ICON(UIIcon::Chess, chess);
    HOME_GRID_ICON(UIIcon::Checkers, checkers);
    HOME_GRID_ICON(UIIcon::Go, go);
    HOME_GRID_ICON(UIIcon::TicTacToe, tictactoe);
    HOME_GRID_ICON(UIIcon::Clock, clock);
    HOME_ICON(UIIcon::Battery, battery);
    HOME_ICON(UIIcon::StorageIcon, storage);
    default:
      return nullptr;
  }
#undef HOME_ICON
#undef HOME_GRID_ICON
}
