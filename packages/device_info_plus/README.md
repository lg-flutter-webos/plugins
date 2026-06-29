# device_info_plus_webos

The webOS implementation of [`device_info_plus`](https://pub.dev/packages/device_info_plus).

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

This package is not an _endorsed_ implementation of `device_info_plus`.
Therefore, you have to include `device_info_plus_webos` alongside `device_info_plus` as dependencies in your `pubspec.yaml` file.

**pubspec.yaml:**

```yaml
dependencies:
  device_info_plus: ^11.1.0
  device_info_plus_webos: ^1.0.0
```

**Import:**

```dart
import 'package:device_info_plus/device_info_plus.dart';
```

## Required configurations

Add the following ACGs to your `appinfo.json`:

```json
{
  "requiredACG": ["systemconfig.query"]
}
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for detailed documentation of all classes, methods, and parameters.
