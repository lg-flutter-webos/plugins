# API Reference — url_launcher_webos

> **Note:** For standard API documentation, see [url_launcher on pub.dev](https://pub.dev/documentation/url_launcher/latest/).
> This document covers webOS-specific behavior only.

## Features

- URL launching for http/https and webOS app launching via data scheme
- URI scheme validation with `canLaunchUrl`
- Launch mode support (platformDefault, externalApplication)

## Overview

This plugin provides the webOS platform implementation of `url_launcher`. Users interact with the standard `url_launcher` API — this package is registered automatically and requires no direct usage.

## Supported APIs

| API | Supported | Notes |
|-----|-----------|-------|
| `canLaunchUrl(Uri url)` | Yes | Checks for `http`, `https` schemes |
| `launchUrl(Uri url)` | Yes | Launches URL |
| `supportsLaunchMode(LaunchMode mode)` | Partial | Only `platformDefault` and `externalApplication` |
| `supportsCloseForLaunchMode(LaunchMode mode)` | No | Always returns `false` |

## Supported URI Schemes

| Scheme | Description |
|--------|-------------|
| `http` / `https` | Opens URL in the webOS browser |

## webOS-Specific: Launching Apps via Data URI

On webOS, `data` scheme URIs are used to launch installed applications:

```dart
// Launch an app by ID
launchUrl(Uri.dataFromString('com.webos.app.browser'));

// Launch an app with parameters (base64 encoded)
launchUrl(Uri.dataFromString(
  '{"id":"com.webos.app.browser","params":{"target":"https://example.com"}}',
  base64: true,
));
```
