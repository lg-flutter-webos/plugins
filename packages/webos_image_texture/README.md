# webos_image_texture

A Flutter widget for displaying images on webOS using GPU texture rendering. Supports PNG, JPG, GIF, WebP, SVG, ETC1, ETC2, ASTC, and animated formats (animated GIF, WebP, APNG) with runtime transcoding.

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

**pubspec.yaml:**

```yaml
dependencies:
  webos_image_texture: ^1.0.0
```

**Import:**

```dart
import 'package:webos_image_texture/image_texture.dart';
```

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for detailed documentation of all classes, methods, and parameters.

## Limitations

- Images larger than 4 megapixels will generate an error.
- Runtime transcoding (PNG, JPG, WebP, SVG to ETC1/ETC2) applies only to images larger than 130x130 pixels.

## Notes

- The `ImageTextureCache` class provides configurable cache management (default 20 MB).
- For animated images, use `play()`, `pause()`, `stop()`, and `setLooping()` to control playback.
