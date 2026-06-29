# url_launcher_webos

The webOS implementation of [`url_launcher`](https://pub.dev/packages/url_launcher).

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

This package is not an _endorsed_ implementation of `url_launcher`.
Therefore, you have to include `url_launcher_webos` alongside `url_launcher` as dependencies in your `pubspec.yaml` file.

**pubspec.yaml:**

```yaml
dependencies:
  url_launcher: ^6.3.1
  url_launcher_webos: ^1.0.0
```

**Import:**

```dart
import 'package:url_launcher/url_launcher.dart';
```

For detailed usage, see https://pub.dev/packages/url_launcher#usage.

## Required configurations

Add the following ACGs to your `appinfo.json`:

```json
"requiredACG": [
  "application.launcher"
]
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for webOS-specific behavior details.

## Limitations

- Only `http`, `https` URI schemes are supported.
- `PreferredLaunchMode.inAppWebView` is not supported. Only `platformDefault` and `externalApplication` modes are available.
- Close for launched URL is not supported.
