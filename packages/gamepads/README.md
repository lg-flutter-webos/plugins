# gamepads_webos

The webOS implementation of [`gamepads`](https://pub.dev/packages/gamepads).

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

This package is not an _endorsed_ implementation of `gamepads`.
Therefore, you have to include `gamepads_webos` alongside `gamepads` as dependencies in your `pubspec.yaml` file.

**pubspec.yaml:**

```yaml
dependencies:
  gamepads: ^0.1.2
  gamepads_webos: ^1.0.0
```

**Import:**

```dart
import 'package:gamepads/gamepads.dart';
```

## Required configurations

Add the following to your `appinfo.json`:

```json
{
  "cloudgame_active": true
}
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for detailed documentation of all classes, methods, and parameters.

## Limitations

- Gamepads without a known Chromium standard mapping are silently excluded from `listGamepads()`.
- Only joystick devices (`/dev/input/jsN`) are detected. Other input device types are filtered out.
- Analog axes are normalized to `-1.0` to `1.0`. Values below ±0.01 deadzone are coerced to `0.0`.
