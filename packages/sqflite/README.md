# sqflite_webos

The webOS implementation of [`sqflite`](https://pub.dev/packages/sqflite).

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

This package is not an _endorsed_ implementation of `sqflite`.
Therefore, you have to include `sqflite_webos` alongside `sqflite` as dependencies in your `pubspec.yaml` file.

**pubspec.yaml:**

```yaml
dependencies:
  sqflite: ^2.4.0
  sqflite_webos:
    git:
      url: https://github.com/lg-flutter-webos/plugins.git
      path: packages/sqflite
      ref: main
```

**Import:**

```dart
import 'package:sqflite/sqflite.dart';
```

For detailed usage, see https://pub.dev/packages/sqflite#usage-example.

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for webOS-specific behavior details.
