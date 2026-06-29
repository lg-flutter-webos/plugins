// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_BLOCKLIST_H_
#define DEVICE_GAMEPAD_GAMEPAD_BLOCKLIST_H_

#include <stdint.h>

namespace gamepad {

// Returns true if a device with IDs matching |vendor_id| and |product_id|
// should not be treated as a gamepad. This is used to exclude devices that
// would otherwise be treated as a gamepad because they expose gamepad-like
// HID usages.
bool GamepadIsExcluded(uint16_t vendor_id,
                       uint16_t product_id);

}  // namespace gamepad

#endif  // DEVICE_GAMEPAD_GAMEPAD_BLOCKLIST_H_
