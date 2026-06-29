# API Reference — flutter_keyboard_visibility_webos

> **Note:** For standard API documentation, see [flutter_keyboard_visibility on pub.dev](https://pub.dev/documentation/flutter_keyboard_visibility/latest/).
> This document covers webOS-specific behavior only.

## Features

- Real-time virtual keyboard visibility monitoring via system method channel
- Stream-based notification on keyboard show/hide events

## Overview

This plugin provides the webOS platform implementation of `flutter_keyboard_visibility`. Users interact with the standard `flutter_keyboard_visibility` API — this package is registered automatically and requires no direct usage.

## Supported APIs

| API | Supported | Notes |
|-----|-----------|-------|
| `FlutterKeyboardVisibility.onChange` | Yes | Emits `true` on show, `false` on hide |
| `FlutterKeyboardVisibility.isVisible` | Yes | Returns current keyboard visibility |

## webOS-Specific Behavior

- Keyboard visibility is monitored through the webOS system method channel.
- The `onChange` stream emits `true` when the virtual keyboard appears and `false` when it is dismissed.
