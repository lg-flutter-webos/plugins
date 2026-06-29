# package_info_plus_webos

The webOS implementation of [`package_info_plus`](https://pub.dev/packages/package_info_plus).

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

This package is not an _endorsed_ implementation of `package_info_plus`.
Therefore, you have to include `package_info_plus_webos` alongside `package_info_plus` as dependencies in your `pubspec.yaml` file.

**pubspec.yaml:**

```yaml
dependencies:
  package_info_plus: ^4.0.1
  package_info_plus_webos: ^1.0.0
```

**Import:**

```dart
import 'package:package_info_plus/package_info_plus.dart';
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for webOS-specific behavior details.

## Limitations

- `PackageInfo.buildSignature` is not supported (returns empty string).
- `PackageInfo.installerStore` is not supported.
