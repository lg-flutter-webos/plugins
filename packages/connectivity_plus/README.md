# connectivity_plus_webos

The webOS implementation of [`connectivity_plus`](https://pub.dev/packages/connectivity_plus).

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

This package is not an _endorsed_ implementation of `connectivity_plus`.
Therefore, you have to include `connectivity_plus_webos` alongside `connectivity_plus` as dependencies in your `pubspec.yaml` file.

**pubspec.yaml:**

```yaml
dependencies:
  connectivity_plus: ^6.1.0
  connectivity_plus_webos:
    git:
      url: https://github.com/lg-flutter-webos/plugins.git
      path: packages/connectivity_plus
      ref: main
```

**Import:**

```dart
import 'package:connectivity_plus/connectivity_plus.dart';
```

For detailed usage, see https://pub.dev/packages/connectivity_plus#usage.

## Required configurations

Add the following ACGs to your `appinfo.json`:

```json
"requiredACG": [
  "network.query"
]
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for webOS-specific behavior details.

## Limitations

- `ConnectivityResult.vpn` is not supported (webOS `connectionmanager` does not provide VPN status).
- `ConnectivityResult.satellite` is not supported.
