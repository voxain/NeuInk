#pragma once

#include <math.h>

#include "../../FreeInkUICore.h"

namespace freeink {
namespace ui {

// Fills the squircle inscribed in `rect`: the superellipse |x|^4 + |y|^4 = 1, rounder at the corners than a
// rounded square and flatter on the sides. One pixel row at a time, so any Paint works (a dither too).
inline void fillSquircle(DrawTarget &target, Rect rect, Paint paint) {
  if (rect.width <= 0 || rect.height <= 0)
    return;
  const float rx = static_cast<float>(rect.width) / 2.0f;
  const float ry = static_cast<float>(rect.height) / 2.0f;
  for (int16_t row = 0; row < rect.height; ++row) {
    const float t = (static_cast<float>(row) + 0.5f - ry) / ry;
    const float t2 = t * t;
    const float rest = 1.0f - t2 * t2;
    const int16_t half = static_cast<int16_t>(lroundf(rx * sqrtf(sqrtf(rest > 0.0f ? rest : 0.0f))));
    if (half <= 0)
      continue;
    target.fill(Rect{static_cast<int16_t>(rect.x + static_cast<int16_t>(rx) - half),
                     static_cast<int16_t>(rect.y + row),
                     static_cast<int16_t>(half * 2), 1},
                paint);
  }
}

} // namespace ui
} // namespace freeink
