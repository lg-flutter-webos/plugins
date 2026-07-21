# shared_preferences_webos

The webOS implementation of [`shared_preferences`](https://pub.dev/packages/shared_preferences).

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

This package is not an _endorsed_ implementation of `shared_preferences`.
Therefore, you have to include `shared_preferences_webos` alongside `shared_preferences` as dependencies in your `pubspec.yaml` file.

**pubspec.yaml:**

```yaml
dependencies:
  shared_preferences: ^2.3.2
  shared_preferences_webos:
    git:
      url: https://github.com/lg-flutter-webos/plugins.git
      path: packages/shared_preferences
      ref: main
```

**Import:**

```dart
import 'package:shared_preferences/shared_preferences.dart';
```

For detailed usage, see https://pub.dev/packages/shared_preferences#usage.

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for webOS-specific behavior details.

## Notes

- Both the legacy `SharedPreferences` API and the newer `SharedPreferencesAsync` API are supported.
- Data is stored via a platform method channel using JSON serialization.
