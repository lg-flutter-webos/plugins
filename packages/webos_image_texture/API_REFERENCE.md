# API Reference — webos_image_texture

## Features

- GPU texture rendering for optimized image display
- Supports animated formats (GIF, WebP, APNG) with play/stop control
- Runtime transcoding to GPU-compressed formats (ETC1/ETC2)
- Configurable texture cache management
- Multiple image sources: asset, network, file, memory

## API Overview

### Classes

- **ImageTexture** — A widget that displays an image using GPU texture rendering. Supports asset, network, file, and memory sources.
- **ImageTextureCache** — Manages the GPU texture cache size.

### Enums

- **DataSourceType** — The type of image data source (asset, network, file, memory).

---

## Classes

### ImageTexture

A `StatefulWidget` that displays an image using GPU texture rendering. Supports various image sources and animated formats.

#### Constructors

##### ImageTexture.asset

```dart
ImageTexture.asset(
  String src, {
  Key? key,
  ImageErrorWidgetBuilder? errorBuilder,
  ImageFrameBuilder? frameBuilder,
  BoxFit? fit,
  AlignmentGeometry alignment = Alignment.center,
  bool matchTextDirection = false,
  int? cacheWidth,
  int? cacheHeight,
  double? width,
  double? height,
  Color? color,
  BlendMode? colorBlendMode,
  bool gaplessPlayback = false,
  String? semanticLabel,
  bool excludeFromSemantics = false,
})
```

Creates an image texture from a Flutter asset.

##### ImageTexture.network

```dart
ImageTexture.network(
  String src, {
  Key? key,
  ImageErrorWidgetBuilder? errorBuilder,
  ImageFrameBuilder? frameBuilder,
  ImageLoadingBuilder? loadingBuilder,
  BoxFit? fit,
  AlignmentGeometry alignment = Alignment.center,
  bool matchTextDirection = false,
  int? cacheWidth,
  int? cacheHeight,
  double? width,
  double? height,
  Color? color,
  BlendMode? colorBlendMode,
  bool gaplessPlayback = false,
  String? semanticLabel,
  bool excludeFromSemantics = false,
})
```

Creates an image texture from a network URL.

##### ImageTexture.file

```dart
ImageTexture.file(
  File file, {
  Key? key,
  ImageErrorWidgetBuilder? errorBuilder,
  ImageFrameBuilder? frameBuilder,
  BoxFit? fit,
  AlignmentGeometry alignment = Alignment.center,
  bool matchTextDirection = false,
  int? cacheWidth,
  int? cacheHeight,
  double? width,
  double? height,
  Color? color,
  BlendMode? colorBlendMode,
  bool gaplessPlayback = false,
  String? semanticLabel,
  bool excludeFromSemantics = false,
})
```

Creates an image texture from a local file.

##### ImageTexture.memory

```dart
ImageTexture.memory(
  Uint8List bytes, {
  Key? key,
  ImageErrorWidgetBuilder? errorBuilder,
  ImageFrameBuilder? frameBuilder,
  BoxFit? fit,
  AlignmentGeometry alignment = Alignment.center,
  bool matchTextDirection = false,
  int? cacheWidth,
  int? cacheHeight,
  double? width,
  double? height,
  Color? color,
  BlendMode? colorBlendMode,
  bool gaplessPlayback = false,
  String? semanticLabel,
  bool excludeFromSemantics = false,
})
```

Creates an image texture from in-memory bytes.

#### Common Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| src / file / bytes | `String` / `File` / `Uint8List` | Yes | The image source |
| errorBuilder | `ImageErrorWidgetBuilder?` | No | Builder for error state |
| frameBuilder | `ImageFrameBuilder?` | No | Builder called when image frame is available |
| loadingBuilder | `ImageLoadingBuilder?` | No | Builder for loading state (network only) |
| fit | `BoxFit?` | No | How to fit the image in the widget (default: `scaleDown`) |
| alignment | `AlignmentGeometry` | No | Alignment within the widget (default: `center`) |
| matchTextDirection | `bool` | No | Mirror image for RTL locales (default: `false`) |
| cacheWidth | `int?` | No | Target cache width for transcoding |
| cacheHeight | `int?` | No | Target cache height for transcoding |
| width | `double?` | No | Widget width |
| height | `double?` | No | Widget height |
| color | `Color?` | No | Color filter to apply |
| colorBlendMode | `BlendMode?` | No | Blend mode for color filter (default: `srcIn`) |
| gaplessPlayback | `bool` | No | Keep old image until new one is ready (default: `false`) |

#### Methods

##### size

```dart
Future<Size?> size()
```

Returns the original size of the image.

##### play

```dart
Future<void> play()
```

Starts or resumes animation playback (for animated GIF, WebP, APNG). Accessed via `controller.play()`.

##### stop

```dart
Future<void> stop()
```

Stops animation playback. Accessed via `controller.stop()`.

---

### ImageTextureCache

Manages the GPU texture cache size.

#### Properties

| Property | Type | Description |
|----------|------|-------------|
| maximumSizeBytes | `int` | Maximum cache size in bytes (default: 20 MB) |

#### Methods

##### clear

```dart
void clear()
```

Clears all cached textures.

---

## Enums

### DataSourceType

The type of image data source.

| Value | Description |
|-------|-------------|
| `asset` | Flutter asset |
| `network` | Network URL |
| `file` | Local file |
| `memory` | In-memory bytes |
| `unknown` | Unknown source |
