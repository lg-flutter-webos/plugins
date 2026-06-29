# API Reference — firebase_remote_config_webos

> **Note:** For standard API documentation, see [`firebase_remote_config` on pub.dev](https://pub.dev/packages/firebase_remote_config).
> This document covers webOS-specific behavior only.

## Features

- Fetch and activate remote configuration values
- Typed value retrieval (String, bool, int, double)
- Default values configuration
- Config settings (fetch timeout, minimum fetch interval)
- Value source tracking (remote, default, static)
- Firebase Remote Config emulator support

## Overview

This plugin provides the webOS platform implementation of `firebase_remote_config`.
It implements `FirebaseRemoteConfigPlatform` and uses Pigeon-generated platform channels
to communicate with the Firebase C++ Remote Config SDK.

## Supported APIs

### Fetch & Activate

| API | Supported | Notes |
|-----|-----------|-------|
| `ensureInitialized()` | Yes | |
| `fetch()` | Yes | Respects minimum fetch interval |
| `activate()` | Yes | Returns `true` if values were activated |
| `fetchAndActivate()` | Yes | Combined fetch + activate |

### Value Retrieval

All typed getters are supported:

| API | Supported | Default (if key not found) |
|-----|-----------|---------------------------|
| `getString()` | Yes | `""` (empty string) |
| `getBool()` | Yes | `false` |
| `getInt()` | Yes | `0` |
| `getDouble()` | Yes | `0.0` |
| `getValue()` | Yes | Returns `RemoteConfigValue` with source info |
| `getAll()` | Yes | Returns all key-value pairs |

### Configuration

| API | Supported | Notes |
|-----|-----------|-------|
| `setDefaults()` | Yes | |
| `setConfigSettings()` | Yes | `fetchTimeout` and `minimumFetchInterval` |
| `setCustomSignals()` | No | No-op (method exists but does nothing) |

### State

| API | Supported | Notes |
|-----|-----------|-------|
| `lastFetchTime` | Yes | |
| `lastFetchStatus` | Yes | Values: `noFetchYet`, `success`, `failure`, `throttle` |
| `settings` | Yes | Current fetch timeout and minimum interval |
| `onConfigUpdated` | No | Returns empty stream (real-time updates not supported) |

## webOS-Specific Behavior

- **No real-time updates**: `onConfigUpdated` returns an empty stream.
  Use `fetchAndActivate()` to poll for new values.
- **Custom signals ignored**: `setCustomSignals()` is a no-op in the current implementation.
- **Value source tracking**: Each retrieved value includes a `ValueSource` indicator
  (`valueRemote`, `valueDefault`, or `valueStatic`) showing where the value came from.
- **Internal caching**: Dart-side cache is refreshed after every fetch/activate operation
  by re-reading all values from the native layer.
