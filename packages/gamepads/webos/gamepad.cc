#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/joystick.h>
#include <libudev.h>
#include <unistd.h>
#include <cstdio>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <map>

#include "gamepad.h"
#include "gamepad_blocklist.h"
#include "gamepad_standard_mappings.h"
#include "log.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "Gamepad::"

namespace gamepad {

/**************************************************/
/* get_gamepad_info(const std::string& device_id) */
/**************************************************/
static constexpr size_t kStringDescriptorMax = 256;

static std::string ToString(const char* cstring) {
  return cstring ? cstring : std::string();
}

static uint16_t HexStringToUInt16(const std::string& in) {
  uint16_t out = 0;
  if (!in.empty()) {
    std::stringstream ss;
    ss << std::hex << in;
    ss >> out;
  }
  return out;
}

static gamepad::GamepadStandardMappingFunction
    get_mapping_function(const std::string& sysname, int fd) {
  struct udev *udev = udev_new();
  if (udev == nullptr) {
    return nullptr;
  }
  struct udev_device* dev = udev_device_new_from_subsystem_sysname(udev, "input", sysname.c_str());
  if (!dev) {
    udev_unref(udev);
    return nullptr;
  }
  dev = udev_device_get_parent_with_subsystem_devtype(dev, "input", nullptr);
  if (!dev) {
    udev_unref(udev);
    return nullptr;
  }

  std::string name = ToString(udev_device_get_sysattr_value(dev, "name"));
  std::string vendor = ToString(udev_device_get_sysattr_value(dev, "id/vendor"));
  std::string product = ToString(udev_device_get_sysattr_value(dev, "id/product"));
  std::string hid_version = ToString(udev_device_get_sysattr_value(dev, "id/version"));
  uint16_t vendor_int = HexStringToUInt16(vendor);
  uint16_t product_int = HexStringToUInt16(product);
  uint16_t hid_version_int = HexStringToUInt16(hid_version);
  uint16_t version_number_int = 0;

  // In many cases the information the input subsystem contains isn't
  // as good as the information that the device bus has, walk up further
  // to the subsystem/device type "usb"/"usb_device" and if this device
  // has the same vendor/product id, prefer the description from that.
  struct udev_device* usb_device =
      udev_device_get_parent_with_subsystem_devtype(
          dev, "usb", "usb_device");
  if (usb_device) {
    std::string usb_vendor =
        ToString(udev_device_get_sysattr_value(usb_device, "idVendor"));
    std::string usb_product =
        ToString(udev_device_get_sysattr_value(usb_device, "idProduct"));

    if (vendor == usb_vendor && product == usb_product) {
      const char* manufacturer =
          udev_device_get_sysattr_value(usb_device, "manufacturer");
      const char* product =
          udev_device_get_sysattr_value(usb_device, "product");

      if (manufacturer && product) {
        // Replace the previous name string with one containing the better
        // information.
        std::stringstream ss;
        ss << manufacturer << " " << product;
        name = ss.str();
      }
    }

    const std::string version_number =
        ToString(udev_device_get_sysattr_value(usb_device, "bcdDevice"));
    version_number_int = HexStringToUInt16(version_number);
  }

  udev_device_unref(dev);
  udev_unref(udev);

  if (vendor_int == 0 && product_int == 0) {
    return nullptr;
  }
  if (GamepadIsExcluded(vendor_int, product_int)) {
    return nullptr;
  }

  gamepad::GamepadBusType bus_type = gamepad::GAMEPAD_BUS_UNKNOWN;
  struct input_id input_info;
  if (ioctl(fd, EVIOCGID, &input_info) >= 0) {
    if (input_info.bustype == BUS_USB)
      bus_type = gamepad::GAMEPAD_BUS_USB;
    if (input_info.bustype == BUS_BLUETOOTH)
      bus_type = gamepad::GAMEPAD_BUS_BLUETOOTH;
  }
  return gamepad::GetGamepadStandardMappingFunction(
             name,
             vendor_int,
             product_int,
             hid_version_int,
             version_number_int,
             bus_type);
}

std::optional<GamepadInfo> get_gamepad_info(const std::string& device_id) {
  size_t found = device_id.find("js");
  if (found == std::string::npos) {
    return std::nullopt;
  }

  std::string system_id = device_id.c_str() + found;
  int file_descriptor = open(device_id.c_str(), O_RDONLY);
  if (file_descriptor == -1) {
    LogError(TAG) << "Could not open joystick: " << device_id;
    return std::nullopt;
  }

  gamepad::GamepadStandardMappingFunction mapper = get_mapping_function(system_id, file_descriptor);
  if (!mapper) {
    close(file_descriptor);
    return std::nullopt;
  }
  LogInfo(TAG) << "Standard Mapper is " << gamepad::MappingFunctionToString(mapper);

  char name[kStringDescriptorMax+1];
  name[kStringDescriptorMax] = 0;
  if (ioctl(file_descriptor, JSIOCGNAME(kStringDescriptorMax), name) < 0) {
    strcpy(name, "Unknown");
  }

  Gamepad data;
  return {{device_id, name, file_descriptor, true, mapper, data, data, data}};
}

/**************************************************/
/*                listen                          */
/**************************************************/
static bool IsDiff(GamepadInfo* gamepad);
const float kMaxLinuxAxisValue = 32767.0;

void listen(GamepadInfo* gamepad,
            const std::function<void(const js_event&)>& event_consumer) {
  while (gamepad->alive) {
    struct js_event event;
    Gamepad* in = &gamepad->in;

    if (read(gamepad->file_descriptor, &event, sizeof(event)) > 0) {
      size_t item = event.number;
      if (event.type & JS_EVENT_AXIS) {
        if (item >= Gamepad::kAxesLengthCap) {
          continue;
        }

        uint32_t mask = 1 << item;
        bool first = (in->axes_used & mask) ? false : true;
        in->axes[item] = event.value / kMaxLinuxAxisValue;
        in->axes_used |= mask;
        if (item >= in->axes_length) {
          in->axes_length = item + 1;
        }
        if (first) {
          continue;
        }
      } else if (event.type & JS_EVENT_BUTTON) {
        if (item >= Gamepad::kButtonsLengthCap) {
          continue;
        }
        bool first = in->buttons[item].used ? false : true;
        in->buttons[item].used = true;
        in->buttons[item].pressed = event.value > 0 ? true: false;
        in->buttons[item].value = event.value;
        if (item >= in->buttons_length) {
          in->buttons_length = item + 1;
        }
        if (first) {
          continue;
        }
      } else {
        continue;
      }
      gamepad->old_out = gamepad->out;
      gamepad->mapper(gamepad->in, &gamepad->out);
      if (IsDiff(gamepad)) {
        event_consumer(event);
      }
    }
  }

  close(gamepad->file_descriptor);
}

/**************************************************/
/*                parse_gamepad_event             */
/**************************************************/
static std::string AxesToString(int indx) {
  static std::map<int, const char*> maps = {
    { gamepad::AXIS_INDEX_LEFT_STICK_X, "LEFT_STICK_X" },
    { gamepad::AXIS_INDEX_LEFT_STICK_Y, "LEFT_STICK_Y" },
    { gamepad::AXIS_INDEX_RIGHT_STICK_X, "RIGHT_STICK_X" },
    { gamepad::AXIS_INDEX_RIGHT_STICK_Y, "RIGHT_STICK_Y" }
  };

  if (maps.find(indx) == maps.end()) {
    std::stringstream ss;
    ss << "UNKNOWN_AXIS" << indx - gamepad::AXIS_INDEX_COUNT;
    return ss.str();
  }
  return maps[indx];
}

static std::string ButtonToString(int indx) {
  static std::map<int, const char*> maps = {
    { gamepad::BUTTON_INDEX_PRIMARY, "A" },
    { gamepad::BUTTON_INDEX_SECONDARY, "B" },
    { gamepad::BUTTON_INDEX_TERTIARY, "X" },
    { gamepad::BUTTON_INDEX_QUATERNARY, "Y" },
    { gamepad::BUTTON_INDEX_LEFT_SHOULDER, "LEFT_SHOULDER" },
    { gamepad::BUTTON_INDEX_RIGHT_SHOULDER, "RIGHT_SHOULDER" },
    { gamepad::BUTTON_INDEX_LEFT_TRIGGER, "LEFT_TRIGGER" },
    { gamepad::BUTTON_INDEX_RIGHT_TRIGGER, "RIGHT_TRIGGER" },
    { gamepad::BUTTON_INDEX_BACK_SELECT, "BACK" },
    { gamepad::BUTTON_INDEX_START, "START" },
    { gamepad::BUTTON_INDEX_LEFT_THUMBSTICK, "LEFT_THUMBSTICK" },
    { gamepad::BUTTON_INDEX_RIGHT_THUMBSTICK, "RIGHT_THUMBSTICK"},
    { gamepad::BUTTON_INDEX_DPAD_UP, "DPAD_UP"},
    { gamepad::BUTTON_INDEX_DPAD_DOWN, "DPAD_DOWN"},
    { gamepad::BUTTON_INDEX_DPAD_LEFT,"DPAD_LEFT"},
    { gamepad::BUTTON_INDEX_DPAD_RIGHT, "DPAD_RIGHT"},
    { gamepad::BUTTON_INDEX_META, "META"},
  };

  if (maps.find(indx) == maps.end()) {
    std::stringstream ss;
    ss << "UNKNOWN_BUTTON" << indx - gamepad::BUTTON_INDEX_COUNT;
    return ss.str();
  }

  return maps[indx];
}

static bool IsDiff(GamepadInfo* gamepad) {
  Gamepad& prev = gamepad->old_out;
  Gamepad& cur = gamepad->out;

  for (unsigned i = 0; i < gamepad->out.buttons_length; i++) {
    if (prev.buttons[i].pressed != cur.buttons[i].pressed) {
      return true;
    }
  }

  for (unsigned i = 0; i < gamepad->out.axes_length; i++) {
    if (prev.axes[i] != cur.axes[i]) {
      return true;
    }
  }
  return false;
}

EventInfo parse_gamepad_event(GamepadInfo* gamepad, const js_event& event) {
  int time = static_cast<int>(event.time);
  std::string type = "button";
  std::string key = "unknown";
  double value = 0;

  const gamepad::Gamepad& prev = gamepad->old_out;
  const gamepad::Gamepad& cur = gamepad->out;

  for (unsigned i = 0; i < gamepad->out.buttons_length; i++) {
    if (prev.buttons[i].pressed != cur.buttons[i].pressed) {
      type = "button";
      key = ButtonToString(i);
      value = cur.buttons[i].pressed ? 1.0 : 0.0;
      return { time, type, key, value };
    }
  }

  for (unsigned i = 0; i < gamepad->out.axes_length; i++) {
    if (prev.axes[i] != cur.axes[i]) {
      type = "analog";
      key = AxesToString(i);
      value = cur.axes[i];
      return { time, type, key, value };
    }
  }

  return { time, type, key, value };
}

/**************************************************/
/*                From Chromium                   */
/**************************************************/
const float GamepadButton::kDefaultButtonPressedThreshold;
const size_t Gamepad::kAxesLengthCap;
const size_t Gamepad::kButtonsLengthCap;

Gamepad::Gamepad()
    : axes_length(0),
      axes_used(0),
      buttons_length(0) {
}

Gamepad::Gamepad(const Gamepad& other) = default;

Gamepad& Gamepad::operator=(const Gamepad& other) = default;

}  // namespace gamepad
