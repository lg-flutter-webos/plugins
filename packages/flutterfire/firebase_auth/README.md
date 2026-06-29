# firebase_auth_webos

The webOS implementation of [`firebase_auth`](https://pub.dev/packages/firebase_auth).

## Supported platforms

This plugin is supported on webOS 26 media or above.

> **Beta**: This plugin relies on the [Firebase C++ SDK for desktop platforms](https://firebase.google.com/docs/cpp/setup?platform=android#libraries-desktop), which Google currently provides as a beta feature.

## Usage

Add both `firebase_auth` and `firebase_auth_webos` as dependencies in your `pubspec.yaml`:

```yaml
dependencies:
  firebase_core: ^3.0.0
  firebase_core_webos: ^1.0.0
  firebase_auth: any
  firebase_auth_webos: ^1.0.0
```

Then import the upstream package:

```dart
import 'package:firebase_auth/firebase_auth.dart';
```

The webOS implementation is automatically used when running on webOS devices.

## Example

For a complete example app, see the [`example`](example/) directory.

## API Reference

See [API_REFERENCE.md](API_REFERENCE.md) for webOS-specific API details.

## Limitations

- Phone authentication is not supported (Firebase C++ SDK limitation).
- OAuth/popup/redirect sign-in is not supported (not applicable to TV).
- Email link (passwordless) authentication is not supported.
- Multi-Factor Authentication is not supported.
- `ActionCodeSettings` parameter is ignored in `sendPasswordResetEmail()` and `sendEmailVerification()`.
