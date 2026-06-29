// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gamepad_standard_mappings.h"

namespace gamepad {

namespace {

const float kButtonAxisDeadzone = 0.01f;

GamepadButton ValueToButton(float value) {
  bool pressed = value > GamepadButton::kDefaultButtonPressedThreshold;
  bool touched = value > 0.f;
  return GamepadButton(pressed, touched, value);
}

}  // namespace

GamepadButton AxisToButton(float input) {
  float value = (input + 1.f) / 2.f;
  return ValueToButton(value);
}

GamepadButton AxisNegativeAsButton(float input) {
  float value = input < -kButtonAxisDeadzone ? -input : 0.f;
  return ValueToButton(value);
}

GamepadButton AxisPositiveAsButton(float input) {
  float value = input > kButtonAxisDeadzone ? input : 0.f;
  return ValueToButton(value);
}

GamepadButton ButtonFromButtonAndAxis(GamepadButton button, float axis) {
  float value = (axis + 1.f) / 2.f;
  return GamepadButton(button.pressed, button.touched, value);
}

GamepadButton NullButton() {
  return GamepadButton();
}

void MapperSwitchPro(const Gamepad& input, Gamepad* mapped) {
  *mapped = input;
  mapped->buttons_length = SWITCH_PRO_BUTTON_COUNT;
  mapped->axes_length = AXIS_INDEX_COUNT;
}

void MapperSwitchJoyCon(const Gamepad& input, Gamepad* mapped) {
  *mapped = input;
  mapped->buttons_length = BUTTON_INDEX_COUNT;
  mapped->axes_length = 2;
}

void MapperSwitchComposite(const Gamepad& input, Gamepad* mapped) {
  // In composite mode, the inputs from two Joy-Cons are combined to form one
  // virtual gamepad. Some buttons do not have equivalents in the Standard
  // Gamepad and are exposed as extra buttons:
  // * Capture button (Joy-Con L):  BUTTON_INDEX_COUNT
  // * SL (Joy-Con L):              BUTTON_INDEX_COUNT + 1
  // * SR (Joy-Con L):              BUTTON_INDEX_COUNT + 2
  // * SL (Joy-Con R):              BUTTON_INDEX_COUNT + 3
  // * SR (Joy-Con R):              BUTTON_INDEX_COUNT + 4
  constexpr size_t kSwitchCompositeExtraButtonCount = 5;
  *mapped = input;
  mapped->buttons_length =
      BUTTON_INDEX_COUNT + kSwitchCompositeExtraButtonCount;
  mapped->axes_length = AXIS_INDEX_COUNT;
}

}  // namespace gamepad
