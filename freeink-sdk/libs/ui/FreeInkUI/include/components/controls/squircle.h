#pragma once

#include <math.h>

#include "../../FreeInkUICore.h"

namespace freeink {
namespace ui {

// The outlines an icon plate can have.
enum class PlateShape : uint8_t { Squircle, RoundedSquare, Circle, Hexagon };

// Fills `shape` inscribed in `rect`, one pixel row at a time, so any Paint works (a dither too). The Squircle is the
// superellipse |x|^4 + |y|^4 = 1, rounder at the corners than a rounded square and flatter on the sides.
// Use an odd width and height: the shape is then symmetric around its middle pixel, and with a dither the
// left and right edges look the same (an even size leaves their edge columns on opposite pixel parity).
inline void fillPlate(DrawTarget &target, Rect rect, PlateShape shape, Paint paint) {
  if (rect.width <= 0 || rect.height <= 0)
    return;
  const int16_t halfWidth = static_cast<int16_t>(rect.width / 2);
  const int16_t halfHeight = static_cast<int16_t>(rect.height / 2);
  const int16_t centerX = static_cast<int16_t>(rect.x + halfWidth);
  const float rx = static_cast<float>(halfWidth) + 0.5f;
  const float ry = static_cast<float>(halfHeight) + 0.5f;
  const float corner = static_cast<float>(rect.width / 4); // the rounded square's radius
  // A dither is anchored to the middle pixel, so the shape looks the same wherever it sits on the screen.
  target.setPatternOrigin(centerX, static_cast<int16_t>(rect.y + halfHeight));
  for (int16_t row = 0; row < rect.height; ++row) {
    const float t = static_cast<float>(row - halfHeight) / ry;
    int16_t half = halfWidth;
    switch (shape) {
    case PlateShape::Squircle: {
      const float t2 = t * t;
      const float rest = 1.0f - t2 * t2;
      half = static_cast<int16_t>(rx * sqrtf(sqrtf(rest > 0.0f ? rest : 0.0f)));
      break;
    }
    case PlateShape::Circle: {
      const float rest = 1.0f - t * t;
      half = static_cast<int16_t>(rx * sqrtf(rest > 0.0f ? rest : 0.0f));
      break;
    }
    case PlateShape::RoundedSquare: {
      const int16_t edge = row < rect.height - 1 - row ? row : static_cast<int16_t>(rect.height - 1 - row);
      if (edge < corner) {
        const float dy = corner - static_cast<float>(edge) - 0.5f;
        half = static_cast<int16_t>(rx - (corner - sqrtf(corner * corner - dy * dy)));
      }
      break;
    }
    case PlateShape::Hexagon: {
      const float a = t < 0.0f ? -t : t;
      if (a > 0.5f)
        half = static_cast<int16_t>(rx * (1.0f - a) * 2.0f);
      break;
    }
    }
    if (half > halfWidth)
      half = halfWidth;
    if (half < 0)
      half = 0;
    target.fill(Rect{static_cast<int16_t>(centerX - half), static_cast<int16_t>(rect.y + row),
                     static_cast<int16_t>(half * 2 + 1), 1},
                paint);
  }
  target.setPatternOrigin(0, 0);
}

inline void fillSquircle(DrawTarget &target, Rect rect, Paint paint) {
  fillPlate(target, rect, PlateShape::Squircle, paint);
}

} // namespace ui
} // namespace freeink
