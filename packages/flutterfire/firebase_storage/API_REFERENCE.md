# API Reference — firebase_storage_webos

> **Note:** For standard API documentation, see [`firebase_storage` on pub.dev](https://pub.dev/packages/firebase_storage).
> This document covers webOS-specific behavior only.

## Features

- File upload (from file, raw bytes, or string with multiple formats)
- File download (URL retrieval and direct download to file)
- File metadata management (get and update)
- Task-based upload progress with pause/resume/cancel
- Retry time configuration
- Firebase Storage emulator support

## Overview

This plugin provides the webOS platform implementation of `firebase_storage`.
It implements `FirebaseStoragePlatform` and uses Pigeon-generated platform channels
to communicate with the Firebase C++ Storage SDK.

## Supported APIs

### Storage Reference

| API | Supported | Notes |
|-----|-----------|-------|
| `ref([path])` | Yes | |
| `refFromURL(url)` | Yes | Supports `gs://` and `https://firebasestorage.googleapis.com` URLs |

### Upload Operations

| API | Supported | Notes |
|-----|-----------|-------|
| `putFile()` | Yes | |
| `putData()` | Yes | Upload from `Uint8List` |
| `putString()` | Yes | Formats: raw, base64, base64Url, dataUrl |
| `writeToFile()` | Yes | Download to local file |

### Metadata Operations

| API | Supported | Notes |
|-----|-----------|-------|
| `getMetadata()` | Yes | |
| `updateMetadata()` | Yes | |
| `getDownloadURL()` | Yes | |
| `getData()` | Yes | |
| `delete()` | Yes | |

### Task Operations

| API | Supported | Notes |
|-----|-----------|-------|
| `onComplete` | Yes | Future-based completion |
| `snapshotEvents` | Yes | Stream of progress snapshots |
| `pause()` | Yes | |
| `resume()` | Yes | |
| `cancel()` | Yes | |

### List Operations

| API | Supported | Notes |
|-----|-----------|-------|
| `list()` | No | Returns empty result (stub) |
| `listAll()` | No | Returns empty result (stub) |

### Configuration

| API | Supported | Notes |
|-----|-----------|-------|
| `maxOperationRetryTime` | Yes | Get/set |
| `maxUploadRetryTime` | Yes | Get/set |
| `maxDownloadRetryTime` | Yes | Get/set |
| `useStorageEmulator()` | Yes | |

## Supported Metadata Fields

cacheControl, contentDisposition, contentEncoding, contentLanguage, contentType,
and customMetadata (Map) are all supported for both get and update operations.

## webOS-Specific Behavior

- **Task handle management**: Each upload/download operation is assigned a unique
  handle string for native-side task tracking and progress reporting.
- **URL parsing**: `refFromURL()` handles both `gs://bucket/path` and full
  `https://firebasestorage.googleapis.com` URLs with automatic bucket/path extraction.
- **List operations**: `list()` and `listAll()` are defined but return empty results.
  These are stub implementations in the current version.
