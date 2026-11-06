#pragma once

#include <math.h>

#include "../../FreeInkUICore.h"

namespace freeink {
namespace ui {

// Fills the squircle inscribed in `rect`: the superellipse |x|^4 + |y|^4 = 1, rounder at the corners than a
// rounded square and flatter on the sides. One pixel row at a time, so any Paint works (a dither too).
// Use an odd width and height: the shape is then symmetric around its middle pixel, and with a dither the
// left and right edges look the same (an even size leaves their edge columns on opposite pixel parity).
inline void fillSquircle(DrawTarget &target, Rect rect, Paint paint) {
  if (rect.width <= 0 || rect.height <= 0)
    return;
  const int16_t halfWidth = static_cast<int16_t>(rect.width / 2);
  const int16_t halfHeight = static_cast<int16_t>(rect.height / 2);
  const int16_t centerX = static_cast<int16_t>(rect.x + halfWidth);
  const float rx = static_cast<float>(halfWidth) + 0.5f;
  const float ry = static_cast<float>(halfHeight) + 0.5f;
  for (int16_t row = 0; row < rect.height; ++row) {
    const float t = static_cast<float>(row - halfHeight) / ry;
    const float t2 = t * t;
    const float rest = 1.0f - t2 * t2;
    int16_t half = static_cast<int16_t>(rx * sqrtf(sqrtf(rest > 0.0f ? rest : 0.0f)));
    if (half > halfWidth)
      half = halfWidth;
    target.fill(Rect{static_cast<int16_t>(centerX - half), static_cast<int16_t>(rect.y + row),
                     static_cast<int16_t>(half * 2 + 1), 1},
                paint);
  }
}

} // namespace ui
} // namespace freeink
