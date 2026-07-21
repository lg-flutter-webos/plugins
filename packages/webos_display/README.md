# webos_display

webOS display info (rotation degree, panel bounds, visibility, orientation change events).

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

**pubspec.yaml:**

```yaml
dependencies:
  webos_display:
    git:
      url: https://github.com/lg-flutter-webos/plugins.git
      path: packages/webos_display
      ref: main
```

**Import:**

```dart
import 'package:webos_display/webos_display.dart';
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for detailed documentation of all classes, methods, and parameters.

## Notes

- `getRotationDegree()` and `getBounds()` return `null` when the native side has not produced a value yet or the reply payload was malformed. `null` is distinct from a legitimate `0` rotation, which the previous `webos_system` API masked.
- Orientation values published to listeners are restricted to `{0, 90, 180, 270}`; values outside that set are silently dropped.
