# firebase_storage_webos

The webOS implementation of [`firebase_storage`](https://pub.dev/packages/firebase_storage).

## Supported platforms

This plugin is supported on webOS 26 media or above.

> **Beta**: This plugin relies on the [Firebase C++ SDK for desktop platforms](https://firebase.google.com/docs/cpp/setup?platform=android#libraries-desktop), which Google currently provides as a beta feature.

## Usage

Add both `firebase_storage` and `firebase_storage_webos` as dependencies in your `pubspec.yaml`:

```yaml
dependencies:
  firebase_core: ^3.0.0
  firebase_core_webos: ^1.0.0
  firebase_storage: ^12.4.10
  firebase_storage_webos: ^1.0.0
```

Then import the upstream package:

```dart
import 'package:firebase_storage/firebase_storage.dart';
```

The webOS implementation is automatically used when running on webOS devices.

## Example

For a complete example app, see the [`example`](example/) directory.

## API Reference

See [API_REFERENCE.md](API_REFERENCE.md) for webOS-specific API details.

## Limitations

- `list()` and `listAll()` are stub implementations that return empty results.
