# firebase_functions_webos

The webOS implementation of [`cloud_functions`](https://pub.dev/packages/cloud_functions).

## Supported platforms

This plugin is supported on webOS 26 media or above.

> **Beta**: This plugin relies on the [Firebase C++ SDK for desktop platforms](https://firebase.google.com/docs/cpp/setup?platform=android#libraries-desktop), which Google currently provides as a beta feature.

## Usage

Add both `cloud_functions` and `firebase_functions_webos` as dependencies in your `pubspec.yaml`:

```yaml
dependencies:
  firebase_core: ^3.0.0
  firebase_core_webos: ^1.0.0
  cloud_functions: ^5.6.2
  firebase_functions_webos: ^1.0.0
```

Then import the upstream package:

```dart
import 'package:cloud_functions/cloud_functions.dart';
```

The webOS implementation is automatically used when running on webOS devices.

## Example

For a complete example app, see the [`example`](example/) directory.

## API Reference

See [API_REFERENCE.md](API_REFERENCE.md) for webOS-specific API details.

## Limitations

- `HttpsCallable.stream()` is not supported (throws `UnimplementedError`). Only request/response invocation via `call()` is available.
