# API Reference — gamepads_webos

> **Note:** For standard API documentation, see [gamepads on pub.dev](https://pub.dev/documentation/gamepads/latest/).
> This document covers webOS-specific behavior only.

## Features

- Gamepad connection and disconnection detection
- Button and analog stick event reporting
- Support for 100+ known gamepad models via Chromium standard mapping

## Overview

This plugin provides the webOS platform implementation of `gamepads`. Users interact with the standard `gamepads` API — this package must be explicitly added to `pubspec.yaml` alongside `gamepads`.

## Supported APIs

| API | Supported | Notes |
|-----|-----------|-------|
| `listGamepads()` | Yes | Returns connected gamepads with `id` and `name` |
| Gamepad events (button/analog) | Yes | Via event channel |

## Supported Inputs

### Buttons

All 17 standard buttons are supported: A, B, X, Y, shoulders, triggers, thumbsticks, D-pad, BACK, START, META.

### Analog Axes

| Axis | Supported | Notes |
|------|-----------|-------|
| LEFT_STICK_X / LEFT_STICK_Y | Yes | Range: -1.0 to 1.0 |
| RIGHT_STICK_X / RIGHT_STICK_Y | Yes | Range: -1.0 to 1.0 |

## webOS-Specific Behavior

- Uses Linux `inotify` to monitor `/dev/input/` for gamepad device connections and disconnections.
- Button/axis mapping follows the **Chromium standard gamepad mapping** with support for 100+ known gamepad models (Xbox, PlayStation, Switch Pro, etc.).
- Unmapped or unrecognized gamepads are silently excluded.
- Each connected gamepad runs on a dedicated input thread for responsive event handling.
- `appinfo.json` must include `"cloudgame_active": true` to enable gamepad input.
