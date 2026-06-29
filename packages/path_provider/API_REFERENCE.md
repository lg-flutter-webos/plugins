# API Reference — path_provider_webos

> **Note:** For standard API documentation, see [path_provider on pub.dev](https://pub.dev/documentation/path_provider/latest/).
> This document covers webOS-specific behavior only.

## Features

- webOS environment variable-based path resolution with fallback paths
- Automatic directory creation with app ID isolation
- Support for temporary, application data, documents, downloads, and external storage directories

## Overview

This plugin provides the webOS platform implementation of `path_provider`. Users interact with the standard `path_provider` API (e.g., `getTemporaryDirectory()`) — this package is registered automatically and requires no direct usage.

## Supported APIs

| API | Supported | Notes |
|-----|-----------|-------|
| `getTemporaryDirectory()` | Yes | `FLUTTER_TEMP_HOME` or `$FLUTTER_HOME/.cache` |
| `getApplicationSupportDirectory()` | Yes | `FLUTTER_APPDATA_HOME` or `$FLUTTER_HOME/.data` |
| `getApplicationDocumentsDirectory()` | Yes | `FLUTTER_APPDATA_HOME` or `$FLUTTER_HOME/.data` |
| `getDownloadsDirectory()` | Yes | `FLUTTER_DOWNLOAD_HOME` or `$FLUTTER_HOME/.download` |
| `getExternalStorageDirectory()` | Yes | `FLUTTER_EXT_STORAGE_PATH` or `$FLUTTER_HOME/data` |
| `getLibraryDirectory()` | No | |
| `getExternalCacheDirectories()` | No | |
| `getExternalStorageDirectories()` | No | |

## webOS-Specific Behavior

- All paths are determined by webOS environment variables. If the variable is not set, a fallback path relative to `FLUTTER_HOME` is used.
- `FLUTTER_HOME` must be set. A `StateError` is thrown if it is missing.
- Directories are created automatically with the application ID appended for isolation.
- `getApplicationSupportDirectory()` and `getApplicationDocumentsDirectory()` resolve to the same path.
