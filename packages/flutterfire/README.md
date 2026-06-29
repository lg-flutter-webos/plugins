# FlutterFire webOS Plugins

webOS implementations of [FlutterFire](https://firebase.flutter.dev/) plugins, built on the Firebase C++ SDK with [Pigeon](https://pub.dev/packages/pigeon) for type-safe Dart–C++ communication.

## Plugins

| Plugin | Upstream Package |
|--------|-----------------|
| [firebase_core](firebase_core/) | [firebase_core](https://pub.dev/packages/firebase_core) |
| [firebase_auth](firebase_auth/) | [firebase_auth](https://pub.dev/packages/firebase_auth) |
| [firebase_storage](firebase_storage/) | [firebase_storage](https://pub.dev/packages/firebase_storage) |
| [firebase_functions](firebase_functions/) | [cloud_functions](https://pub.dev/packages/cloud_functions) |
| [firebase_remote_config](firebase_remote_config/) | [firebase_remote_config](https://pub.dev/packages/firebase_remote_config) |

## Supported Platforms

All plugins are supported on webOS 26 media or above.

> **Beta**: These plugins rely on the [Firebase C++ SDK for desktop platforms](https://firebase.google.com/docs/cpp/setup?platform=android#libraries-desktop), which Google currently provides as a beta feature.

## Prerequisites

All Firebase plugins require `firebase_core` to be initialized first. See each plugin's README for usage details.
