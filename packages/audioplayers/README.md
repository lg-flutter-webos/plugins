# audioplayers_webos

The WebOS implementation of [`audioplayers`](https://pub.dev/packages/audioplayers).

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

This package is not an _endorsed_ implementation of `audioplayers`.
Therefore, you have to include `audioplayers_webos` alongside `audioplayers`
as dependencies in your `pubspec.yaml` file.

**pubspec.yaml:**

```yaml
dependencies:
  audioplayers: ^6.4.0
  audioplayers_webos:
    git:
      url: https://github.com/lg-flutter-webos/plugins.git
      path: packages/audioplayers
      ref: main
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

- Up to **5 audio players** can run concurrently in a single application.
  When the limit is reached, creating a new `AudioPlayer` automatically
  evicts the least-recently-created one; that player receives an `error`
  event with code `-1` ("num of pipelines") so the Dart side can detect
  the eviction.
- **HLS (m3u8) streams containing a video track are not supported.** The
  player runs as an audio-only consumer and does not provide a video
  window to the platform pipeline, so loading a multi-bitrate HLS
  manifest that includes a video variant leaves the platform AV
  connector uninitialized and no audio is rendered. Audio-only sources
  (`.mp3`, `.wav`, raw MPEG audio HTTP streams, etc.) work as expected.
