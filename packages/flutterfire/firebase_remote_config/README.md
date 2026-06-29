# firebase_remote_config_webos

The webOS implementation of [`firebase_remote_config`](https://pub.dev/packages/firebase_remote_config).

## Supported platforms

This plugin is supported on webOS 26 media or above.

> **Beta**: This plugin relies on the [Firebase C++ SDK for desktop platforms](https://firebase.google.com/docs/cpp/setup?platform=android#libraries-desktop), which Google currently provides as a beta feature.

## Usage

Add both `firebase_remote_config` and `firebase_remote_config_webos` as dependencies in your `pubspec.yaml`:

```yaml
dependencies:
  firebase_core: ^3.0.0
  firebase_core_webos: ^1.0.0
  firebase_remote_config: ^5.5.0
  firebase_remote_config_webos: ^1.0.0
```

Then import the upstream package:

```dart
import 'package:firebase_remote_config/firebase_remote_config.dart';
```

The webOS implementation is automatically used when running on webOS devices.

## Example

For a complete example app, see the [`example`](example/) directory.

## API Reference

See [API_REFERENCE.md](API_REFERENCE.md) for webOS-specific API details.

## Limitations

- `onConfigUpdated` returns an empty stream (real-time config updates not supported). Use `fetchAndActivate()` to poll for new values.
- `setCustomSignals()` is a no-op in the current implementation.
