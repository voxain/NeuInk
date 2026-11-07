#pragma once

#include <FreeInkUICore.h>
#include <I18n.h>
#include <Icon.h>

#include "components/icons/homeIcons.h"

// One icon for every direction: the toolkit draws a bitmap turned in 90 degree steps (Rotation), a coordinate swap
// per pixel, so a chevron facing left, up or down is the right-facing one turned, not a copy of its own.
struct TurnedIcon {
  const freeink::Icon* icon = nullptr;
  freeink::ui::Rotation rotation = freeink::ui::Rotation::None;
};

namespace turned {

using freeink::ui::Rotation;

// A chevron (24 or 32 px), from the one that faces right.
inline TurnedIcon chevronLeft(const int size = 24) {
  return {size == 32 ? &icon_chevron_right_32 : &icon_chevron_right_24, Rotation::R180};
}
inline TurnedIcon chevronUp(const int size = 32) {
  return {size == 32 ? &icon_chevron_right_32 : &icon_chevron_right_24, Rotation::CCW90};
}
inline TurnedIcon chevronDown(const int size = 32) {
  return {size == 32 ? &icon_chevron_right_32 : &icon_chevron_right_24, Rotation::CW90};
}

// What a swipe setting does: an arrow for one finger (up, down), a double chevron for two (all four directions);
// icon nullptr for any other setting.
inline TurnedIcon swipeAction(const StrId name) {
  switch (name) {
    case StrId::STR_LEFT_EDGE_UP:
    case StrId::STR_RIGHT_EDGE_UP:
      return {&icon_arrow_up_24, Rotation::None};
    case StrId::STR_LEFT_EDGE_DOWN:
    case StrId::STR_RIGHT_EDGE_DOWN:
      return {&icon_arrow_up_24, Rotation::R180};
    case StrId::STR_TWO_FINGER_SWIPE_UP:
      return {&icon_chevrons_right_24, Rotation::CCW90};
    case StrId::STR_TWO_FINGER_SWIPE_DOWN:
      return {&icon_chevrons_right_24, Rotation::CW90};
    case StrId::STR_TWO_FINGER_SWIPE_LEFT:
      return {&icon_chevrons_right_24, Rotation::R180};
    case StrId::STR_TWO_FINGER_SWIPE_RIGHT:
      return {&icon_chevrons_right_24, Rotation::None};
    default:
      return {};
  }
}

}  // namespace turned
