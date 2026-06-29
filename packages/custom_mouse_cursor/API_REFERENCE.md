# API Reference — custom_mouse_cursor

> **Note:** For standard API documentation, see [custom_mouse_cursor on pub.dev](https://pub.dev/packages/custom_mouse_cursor).
> This document covers webOS-specific behavior only.

## Features

- Custom system mouse cursors from Flutter image assets with DPR-aware resolution
- Custom cursors from Flutter `IconData` (Material Icons, etc.)
- Custom cursors from `ui.Image` objects with multi-DPR support
- Predefined cursor kinds (blank, system)
- Device Pixel Ratio (DPR)-aware asset resolution and cursor caching

## Overview

This plugin provides custom mouse cursor support on webOS. Users interact with the standard `custom_mouse_cursor` API — this package works on webOS without additional configuration.

## Supported APIs

### Static Factory Methods

| API | Supported | Notes |
|-----|-----------|-------|
| `CustomMouseCursor.asset(...)` | Yes | DPR-aware asset resolution via `AssetImage` |
| `CustomMouseCursor.exactAsset(...)` | Yes | Fixed DPR asset loading via `ExactAssetImage` |
| `CustomMouseCursor.icon(...)` | Yes | Creates cursor from `IconData` with size, color, fill, weight options |
| `CustomMouseCursor.image(...)` | Yes | Creates cursor from `ui.Image` object |
| `CustomMouseCursor.kind(...)` | Yes | Predefined cursor type (`blank`, `system`) |

### Instance Methods

| API | Supported | Notes |
|-----|-----------|-------|
| `addImage(ui.Image, {double thisImagesDevicePixelRatio})` | Yes | Adds DPR variant to `image()`-created cursor |
| `finalizeImages()` | Yes | Finalizes `image()`-created cursor for current DPR |
| `dispose()` | Yes | Frees platform cursor resource |
| `switchToCachedDevicePixelRatioIfPossible(double)` | Yes | Switches to cached DPR variant if available |

### Static Methods

| API | Supported | Notes |
|-----|-----------|-------|
| `disposeAll()` | Yes | Disposes all created cursors |
| `ensurePointersMatchDevicePixelRatio(BuildContext?)` | Yes | Updates all cursors on DPR change |

## Factory Method Parameters

### `exactAsset`

```dart
static Future<CustomMouseCursor> exactAsset(
  String assetName, {
  int hotX = 0,
  int hotY = 0,
  double nativeDevicePixelRatio = 1.0,
  CustomMouseCursor? existingCursorToUpdate,
  BuildContext? context,
  AssetBundle? bundle,
  String? package,
})
```

Loads the asset at the exact path specified (no DPR-aware variant resolution). Delegates to `asset()` with `useExactAssetImage: true`.

### `icon`

```dart
static Future<CustomMouseCursor> icon(
  IconData icon, {
  double size = 32,
  int hotX = 0,
  int hotY = 0,
  double? fill,
  double? weight,
  double? grade,
  double? opticalSize,
  Color color = Colors.black,
  List<Shadow>? shadows,
  CustomMouseCursor? existingCursorToUpdate,
})
```

Creates a cursor from any Flutter `IconData`. The icon is rendered at the specified `size` in logical pixels and scaled automatically for the current DPR.

### `image`

```dart
static Future<CustomMouseCursor> image(
  ui.Image uiImage, {
  int hotX = 0,
  int hotY = 0,
  double thisImagesDevicePixelRatio = 1.0,
  bool finalizeForCurrentDPR = true,
  String? key,
})
```

Creates a cursor from a `ui.Image` object. Use `addImage()` to supply additional DPR variants, then call `finalizeImages()` if `finalizeForCurrentDPR` was set to `false`.

### `kind`

```dart
static Future<CustomMouseCursor> kind(
  String kind, {
  int hotX = 0,
  int hotY = 0,
  double nativeDevicePixelRatio = 1.0,
})
```

Creates a predefined cursor by name. Supported kinds: `blank` (invisible cursor), `system` (default system cursor).
