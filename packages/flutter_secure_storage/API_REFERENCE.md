# API Reference — flutter_secure_storage_webos

> **Note:** For standard API documentation, see [flutter_secure_storage on pub.dev](https://pub.dev/documentation/flutter_secure_storage/latest/).
> This document covers webOS-specific behavior only.

## Features

- Encrypted key-value storage using webOS KeyManager3 service
- AES-256-CBC encryption with per-key encryption keys
- Persistent storage via SQLite database

## Overview

This plugin provides the webOS platform implementation of `flutter_secure_storage`. Users interact with the standard `flutter_secure_storage` API — this package must be explicitly added to `pubspec.yaml` alongside `flutter_secure_storage`.

## Supported APIs

All standard storage operations (`read`, `write`, `delete`, `readAll`, `deleteAll`, `containsKey`) are supported.

| API | Supported | Notes |
|-----|-----------|-------|
| Platform-specific options (iOS/Android) | No | Ignored on webOS |

## webOS-Specific Behavior

- Data is encrypted using **AES-256-CBC** via the webOS KeyManager3 Luna service. Each storage key gets its own generated encryption key.
- Encrypted data is persisted in a SQLite database at `${FLUTTER_APPDATA_HOME}/${FLUTTER_APP_ID}/flutter_secure_storage.db`.
- Platform-specific options for iOS (Keychain) and Android (EncryptedSharedPreferences) are not applicable and are ignored.
- Invalid or corrupted entries are automatically cleaned up during initialization.
