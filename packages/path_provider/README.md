# path_provider_webos

The webOS implementation of [`path_provider`](https://pub.dev/packages/path_provider).

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

This package is not an _endorsed_ implementation of `path_provider`.
Therefore, you have to include `path_provider_webos` alongside `path_provider` as dependencies in your `pubspec.yaml` file.

**pubspec.yaml:**

```yaml
dependencies:
  path_provider: ^2.1.1
  path_provider_webos: ^1.0.0
```

**Import:**

```dart
import 'package:path_provider/path_provider.dart';
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for webOS-specific behavior details.

## Limitations

- `getLibraryDirectory()` is not supported on webOS.
- `getExternalCacheDirectories()` is not supported on webOS.
- `getExternalStorageDirectories()` is not supported on webOS.

## Notes

- Paths are determined by webOS environment variables (`FLUTTER_HOME`, `FLUTTER_TEMP_HOME`, `FLUTTER_APPDATA_HOME`, `FLUTTER_DOWNLOAD_HOME`, `FLUTTER_EXT_STORAGE_PATH`).
- If environment variables are not set, fallback paths relative to `FLUTTER_HOME` are used.
