# API Reference — firebase_core_webos

> **Note:** For standard API documentation, see [`firebase_core` on pub.dev](https://pub.dev/packages/firebase_core).
> This document covers webOS-specific behavior only.

## Features

- Firebase app initialization and lifecycle management on webOS
- Multiple named Firebase app instances
- Thread-safe C++ app caching via Firebase C++ SDK
- Pigeon-based type-safe Dart–C++ communication

## Overview

This plugin provides the webOS platform implementation of `firebase_core`.
It registers as the platform delegate via `FirebasePlatform` and communicates
with the native Firebase C++ SDK through Pigeon-generated platform channels.

## Supported APIs

| API | Supported | Notes |
|-----|-----------|-------|
| `Firebase.initializeApp()` | Yes | `FirebaseOptions` required (throws `ArgumentError` if null). 30-second timeout enforced. |
| `Firebase.app()` | Yes | Retrieves cached app by name |
| `Firebase.apps` | Yes | Returns all initialized apps |
| `FirebaseApp.delete()` | Yes | Removes from both Dart cache and C++ native layer |

All standard `firebase_core` APIs are supported.

## webOS-Specific Behavior

- **FirebaseOptions required**: Unlike some platforms, webOS requires `FirebaseOptions`
  to be explicitly provided at initialization. Omitting options throws `ArgumentError`.
- **Initialization timeout**: `initializeApp()` enforces a 30-second timeout.
  Exceeding this throws `TimeoutException`.
- **Default app naming**: The default app name `[DEFAULT]` has special handling
  in the C++ layer (`firebase::App::Create(options)` vs `firebase::App::Create(options, name)`).
- **Thread-safe caching**: The C++ `FirebaseAppHolder` uses `std::mutex` for
  thread-safe app lifecycle management.

## Supported FirebaseOptions Fields

| Field | Passed to C++ | Notes |
|-------|:---:|-------|
| `apiKey` | Yes | Required |
| `appId` | Yes | Required |
| `projectId` | Yes | Required |
| `messagingSenderId` | Yes | Optional |
| `databaseURL` | Yes | Optional |
| `storageBucket` | Yes | Optional |
| `authDomain` | Yes | Defined in Pigeon but not used by C++ Firebase SDK |

## Error Codes

| Code | Cause |
|------|-------|
| `no-app` | Accessed an app that has not been initialized |
| `INVALID_ARGUMENT` | Empty app name passed to native layer |
| `INIT_FAILED` | `firebase::App::Create()` returned null |
| `APP_NOT_FOUND` | App not found when calling `getApp()` or `delete()` |
