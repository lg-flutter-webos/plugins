#ifndef DEVICE_GAMEPAD_GAMEPAD_H_
#define DEVICE_GAMEPAD_GAMEPAD_H_

#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>
#include <stdint.h>

#include <functional>
#include <optional>
#include <string>

namespace gamepad {

class GamepadButton {
 public:
  static constexpr float kDefaultButtonPressedThreshold = 30.f / 255.f;

  GamepadButton() = default;
  GamepadButton(bool pressed, bool touched, double value)
      : used(true), pressed(pressed), touched(touched), value(value) {}
  bool operator==(const GamepadButton& other) const {
    return this->used == other.used && this->pressed == other.pressed &&
           this->touched == other.touched && this->value == other.value;
  }

  bool used{false};
  bool pressed{false};
  bool touched{false};
  double value{0.0};
};

class Gamepad {
 public:
  static constexpr size_t kAxesLengthCap = 16;
  static constexpr size_t kButtonsLengthCap = 32;

  Gamepad();
  Gamepad(const Gamepad& other);
  Gamepad& operator=(const Gamepad& other);

  unsigned axes_length;
  uint32_t axes_used;
  double axes[kAxesLengthCap];

  unsigned buttons_length;
  GamepadButton buttons[kButtonsLengthCap];
};

typedef void (*MappingFunction)(const Gamepad& in,
                                Gamepad* out);

struct GamepadInfo {
  std::string device_id;
  std::string name;
  int file_descriptor;
  bool alive;

  MappingFunction mapper;
  Gamepad in;
  Gamepad out;
  Gamepad old_out;
};

std::optional<GamepadInfo> get_gamepad_info(const std::string& device_id);

void listen(GamepadInfo* gamepad,
            const std::function<void(const js_event&)>& event_consumer);

struct EventInfo {
  int time;
  std::string type;
  std::string key;
  double value;
};

EventInfo parse_gamepad_event(GamepadInfo* gamepad, const js_event& event);

}  // namespace gamepad

#endif
