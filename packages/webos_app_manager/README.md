# webos_app_manager

Queries app load status from the webOS App Manager.

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

**pubspec.yaml:**

```yaml
dependencies:
  webos_app_manager: ^1.0.0
```

**Import:**

```dart
import 'package:webos_app_manager/webos_app_manager.dart';
```

## Required configurations

Add the following ACGs to your `appinfo.json`:

```json
{
  "requiredACG": [
    "application.launcher",
    "application.query"
  ]
}
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for detailed documentation of all classes, methods, and parameters.
