# API Reference — package_info_plus_webos

> **Note:** For standard API documentation, see [package_info_plus on pub.dev](https://pub.dev/documentation/package_info_plus/latest/).
> This document covers webOS-specific behavior only.

## Features

- App metadata retrieval from `version.json` asset file
- Standard `PackageInfo` fields: appName, version, buildNumber, packageName

## Overview

This plugin provides the webOS platform implementation of `package_info_plus`. Users interact with the standard `PackageInfo.fromPlatform()` API — this package is registered automatically and requires no direct usage.

## Supported Fields

| Field | Type | Supported | Source |
|-------|------|-----------|--------|
| `appName` | `String` | Yes | `version.json` → `app_name` |
| `version` | `String` | Yes | `version.json` → `version` |
| `buildNumber` | `String` | Yes | `version.json` → `build_number` |
| `packageName` | `String` | Yes | `version.json` → `package_name` |
| `buildSignature` | `String` | No | Always returns empty string |
| `installerStore` | `String?` | No | Not available on webOS |

## webOS-Specific Behavior

- Package information is read from a `version.json` asset file bundled with the application.
- If `version.json` is not found or cannot be parsed, all fields return empty strings.
