# custom_mouse_cursor

Custom mouse cursors for Flutter which are devicePixelRatio aware. Forked from [custom_mouse_cursor](https://github.com/timmaffett/custom_mouse_cursor) for webOS support.

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

**pubspec.yaml:**

```yaml
dependencies:
  custom_mouse_cursor: ^1.0.0
```

**Import:**

```dart
import 'package:custom_mouse_cursor/custom_mouse_cursor.dart';
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for detailed documentation of all classes, methods, and parameters.

## Notes

- Cursors are automatically adjusted for the system's devicePixelRatio.
- All cursor work is cached, so switching between monitors with varying devicePixelRatios is seamless.
- Cursor images can be provided at multiple devicePixelRatio variants (1.5x, 2.0x, 2.5x, etc.) for optimal quality.
