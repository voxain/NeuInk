#pragma once

#include "../../FreeInkUICore.h"

namespace freeink {
namespace ui {

// Where an icon sits next to a label.
enum class IconPlacement : uint8_t { Beside, Above };

struct ButtonProps {
  const char* label = nullptr;
  BitmapRef icon{};
  AssetRef iconAsset{};
  ActionId action = NO_ACTION;
  int16_t value = 0;
  uint16_t inputMask = InputDefault;
  State state = StateNormal;
  TextStyle text{};
  StyleSet styles{};
  int16_t minTouchSize = 44;
  // Icon draw size: 0 = the bitmap's native pixels; >0 scales to a square of
  // this size (nearest-neighbor), so one asset serves several button sizes.
  int16_t iconSize = 0;
  // The icon turned in 90 degree steps, drawn that way (no second bitmap needed).
  Rotation iconRotation = Rotation::None;
  // Icon and label side by side (the default) or the label under the icon.
  IconPlacement iconPlacement = IconPlacement::Beside;
  // A label wider than the button is set in this font instead of text.font (0 = never switch).
  FontId shrinkFont = 0;
  uint8_t radius = 0;
  // Extra hit area beyond the visual rect, per edge. Use this to give
  // adjacent controls contiguous, non-overlapping tap bands (split the gap
  // between a stepper's -/+ instead of letting centered minTouchSize
  // expansion overlap the neighbor). Composes with minTouchSize, screen
  // clamping, and edge snapping; the visual rect is unchanged.
  Insets hitPadding{};
  int16_t gap = 4;
  uint8_t borderEdges = EdgesAll;
  bool enabled = true;
  // Highlight insets affect paint only, preserving text and hit geometry.
  Insets highlightInsets{};
  Insets padding{2, 4, 2, 4};
};

// Paints a button without registering a hit area, for screens that draw straight onto a target.
inline void paintButton(DrawTarget& target, AssetResolver* assets, const Rect rect, const ButtonProps& props,
                        const State state) {
  StyleSet styles = props.styles.unset() ? defaultButtonStyles() : props.styles;
  if (props.radius > 0) setStyleRadius(styles, props.radius);
  const BoxStyle& style = styles.resolve(state);
  const bool highlighted = !hasState(state, StateDisabled) &&
                           (hasState(state, StateSelected) || hasState(state, StateActive) ||
                            hasState(state, StateFocused) || hasState(state, StateChecked));
  const Rect backgroundRect = highlighted ? rect.inset(props.highlightInsets) : rect;
  target.fill(backgroundRect, style.background, style.radius, style.corners);
  if (style.border.kind != PaintKind::None && style.borderWidth > 0) {
    drawBorderEdges(target, backgroundRect, style.border, style.borderWidth, style.radius, style.corners,
                    props.borderEdges);
  }

  Rect content = rect.inset(props.padding);
  if (content.empty()) return;
  BitmapRef icon = props.icon ? props.icon : resolveBitmap(assets, props.iconAsset);
  // iconSize > 0 scales the icon (nearest-neighbor Contain) instead of
  // drawing at its native pixel size.
  const int16_t iconW = icon ? (props.iconSize > 0 ? props.iconSize : static_cast<int16_t>(icon.width)) : 0;
  const int16_t iconH = icon ? (props.iconSize > 0 ? props.iconSize : static_cast<int16_t>(icon.height)) : 0;
  const BitmapMode iconMode = props.iconSize > 0 ? BitmapMode::Contain : BitmapMode::Center;
  TextStyle labelStyle = props.text;
  if (props.label && props.shrinkFont != 0 &&
      target.measureText(labelStyle.font, props.label, labelStyle).width > content.width) {
    labelStyle.font = props.shrinkFont;
  }
  if (icon && props.label && props.iconPlacement == IconPlacement::Above) {
    const int16_t labelH = target.measureText(labelStyle.font, props.label, labelStyle).height;
    const int16_t top = static_cast<int16_t>(content.y + (content.height - iconH - props.gap - labelH) / 2);
    target.bitmap(Rect{static_cast<int16_t>(content.x + (content.width - iconW) / 2), top, iconW, iconH}, icon,
                  iconMode, style.foreground, props.iconRotation);
    TextStyle textStyle = textStyleWithForeground(labelStyle, style.foreground);
    textStyle.align = TextAlign::Center;
    target.text(Rect{content.x, static_cast<int16_t>(top + iconH + props.gap), content.width, labelH}, props.label,
                textStyle);
  } else if (icon && props.label) {
    Size labelSize = target.measureText(labelStyle.font, props.label, labelStyle);
    int16_t totalW = static_cast<int16_t>(iconW + props.gap + labelSize.width);
    int16_t x = static_cast<int16_t>(content.x + (content.width - totalW) / 2);
    Rect iconRect{x, static_cast<int16_t>(content.y + (content.height - iconH) / 2), iconW, iconH};
    target.bitmap(iconRect, icon, iconMode, style.foreground, props.iconRotation);
    Rect textRect{static_cast<int16_t>(x + iconW + props.gap), content.y,
                  static_cast<int16_t>(content.right() - x - iconW - props.gap), content.height};
    TextStyle textStyle = textStyleWithForeground(labelStyle, style.foreground);
    textStyle.align = TextAlign::Left;
    target.text(textRect, props.label, textStyle);
  } else if (icon) {
    Rect iconRect = centeredRect(content, Size{iconW, iconH});
    target.bitmap(iconRect, icon, iconMode, style.foreground, props.iconRotation);
  } else if (props.label) {
    TextStyle textStyle = textStyleWithForeground(labelStyle, style.foreground);
    textStyle.align = TextAlign::Center;
    target.text(content, props.label, textStyle);
  }
}

template <size_t MaxInteractions>
void button(Frame<MaxInteractions>& frame, Rect rect, const ButtonProps& props) {
  if (props.enabled && props.action != NO_ACTION) {
    const Rect padded{static_cast<int16_t>(rect.x - props.hitPadding.left),
                      static_cast<int16_t>(rect.y - props.hitPadding.top),
                      static_cast<int16_t>(rect.width + props.hitPadding.left + props.hitPadding.right),
                      static_cast<int16_t>(rect.height + props.hitPadding.top + props.hitPadding.bottom)};
    Rect hitRect = ensureMinTouchRect(padded, props.minTouchSize, frame.screen());
    frame.hit(hitRect, props.action, props.value, props.inputMask, props.state);
  }
  State state = props.enabled ? props.state : static_cast<State>(props.state | StateDisabled);
  state = frame.stateFor(props.action, props.value, state);
  paintButton(frame.target(), frame.assets(), rect, props, state);
}

}  // namespace ui
}  // namespace freeink
