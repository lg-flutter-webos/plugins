# API Reference — device_info_plus_webos

> **Note:** For standard API documentation, see [device_info_plus on pub.dev](https://pub.dev/documentation/device_info_plus/latest/).
> This document covers webOS-specific additions only.

## Features

- webOS-specific device properties: model, display capabilities, hardware info
- Standard `DeviceInfoPlugin` API with additional `WebOSDeviceInfo` class

## Overview

This plugin provides the webOS platform implementation of `device_info_plus`. Users interact with the standard `DeviceInfoPlugin` API — this package is registered automatically and requires no direct usage.

## Supported APIs

| API | Supported | Notes |
|-----|-----------|-------|
| `DeviceInfoPlugin().deviceInfo` | Yes | Returns `WebOSDeviceInfo` on webOS |

## webOS-Specific Classes

### DeviceInfoWebOSPlugin

The webOS platform implementation of `DeviceInfoPlatform`. Registered automatically at app startup.

#### Properties

| Property | Type | Description |
|----------|------|-------------|
| webosInfo | `Future<WebOSDeviceInfo>` | Returns webOS device information, fetching on first call and caching the result. |

---

### WebOSDeviceInfo

Contains detailed information about the webOS device. Implements `BaseDeviceInfo`.

#### Properties

| Property | Type | Description |
|----------|------|-------------|
| data | `Map<String, dynamic>` | Returns all device info as a map. Alias for `toMap()`. |
| modelName | `String?` | TV model name |
| version | `String?` | webOS version string |
| versionMajor | `int?` | webOS major version number |
| versionMinor | `int?` | webOS minor version number |
| versionDot | `int?` | webOS dot version number |
| sdkVersion | `String?` | webOS SDK version |
| screenWidth | `int?` | Display width in pixels |
| screenHeight | `int?` | Display height in pixels |
| uhd | `bool?` | Whether the display supports UHD (4K) |
| oled | `bool?` | Whether the display is OLED |
| ddrSize | `String?` | DDR memory size |
| hdr10 | `bool?` | Whether HDR10 is supported |
| dolbyVision | `bool?` | Whether Dolby Vision is supported |
| dolbyAtmos | `bool?` | Whether Dolby Atmos is supported |
| brandName | `String?` | Brand name |
| manufacturer | `String?` | Manufacturer name |

#### Methods

##### toMap

```dart
Map<String, dynamic> toMap()
```

Returns all device info as a map.
