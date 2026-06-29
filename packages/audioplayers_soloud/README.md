# audioplayers_soloud

The WebOS implementation of the [`audioplayers`](https://pub.dev/packages/audioplayers) plugin based on flutter_soloud.

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

This package is not an _endorsed_ implementation of `audioplayers`.
Therefore, you have to include `audioplayers_soloud` alongside `audioplayers`
as dependencies in your `pubspec.yaml` file.

**pubspec.yaml:**

```yaml
dependencies:
  audioplayers: ^6.4.0
  audioplayers_soloud: ^1.0.0
```

**Import:**

```dart
import 'package:audioplayers/audioplayers.dart';
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for detailed documentation of all classes, methods, and parameters.

## Limitations

- `AudioPlayer.setSourceUrl` supports only static or progressive HTTP(S) files (e.g. fully downloadable `.mp3`, `.wav`). Endless live streams (e.g. radio streams) are not supported, because the implementation downloads the URL into memory before handing it to the decoder. For live stream playback, use `audioplayers_webos` instead.
- HLS (m3u8) manifests are not supported. The implementation has no manifest parser or segment downloader, so the manifest body is handed to the SoLoud decoder which cannot interpret it. HLS with a video track is additionally unsupported by `audioplayers_webos` (audio-only consumer cannot drive a video sink).
