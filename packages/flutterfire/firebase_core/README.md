# firebase_core_webos

The webOS implementation of [`firebase_core`](https://pub.dev/packages/firebase_core).

## Supported platforms

This plugin is supported on webOS 26 media or above.

> **Beta**: This plugin relies on the [Firebase C++ SDK for desktop platforms](https://firebase.google.com/docs/cpp/setup?platform=android#libraries-desktop), which Google currently provides as a beta feature.

## Usage

Add both `firebase_core` and `firebase_core_webos` as dependencies in your `pubspec.yaml`:

```yaml
dependencies:
  firebase_core: ^3.0.0
  firebase_core_webos:
    git:
      url: https://github.com/lg-flutter-webos/plugins.git
      path: packages/flutterfire/firebase_core
      ref: main
```

Then import the upstream package:

```dart
import 'package:firebase_core/firebase_core.dart';
```

The webOS implementation is automatically used when running on webOS devices.

## Example

For a complete example app, see the [`example`](example/) directory.

## API Reference

See [API_REFERENCE.md](API_REFERENCE.md) for webOS-specific API details.

## Limitations

- `FirebaseOptions` must be provided explicitly at initialization (throws `ArgumentError` if omitted).
- App state is in-memory only and not persisted across app restarts.
- Initialization enforces a 30-second timeout.
