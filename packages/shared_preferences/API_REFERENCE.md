# API Reference — shared_preferences_webos

> **Note:** For standard API documentation, see [shared_preferences on pub.dev](https://pub.dev/documentation/shared_preferences/latest/).
> This document covers webOS-specific behavior only.

## Features

- Legacy `SharedPreferences` and async `SharedPreferencesAsync` APIs supported
- Persistent key-value storage with JSON serialization
- All standard data types: String, int, double, bool, List\<String\>

## Overview

This plugin provides the webOS platform implementation of `shared_preferences`. Users interact with the standard `shared_preferences` API — this package is registered automatically and requires no direct usage.

## Supported APIs

### SharedPreferences (legacy)

The legacy API uses a bulk-access pattern. The webOS platform implements the following low-level methods, and the upstream `shared_preferences` package provides the user-facing convenience methods (`getString`, `setString`, etc.) on top of these.

| Platform API | Supported | Notes |
|-----|-----------|-------|
| `getAll()` | Yes | Returns all stored key-value pairs |
| `setValue(String type, String key, Object value)` | Yes | Supports `String`, `Int`, `Double`, `Bool`, `StringList` |
| `remove(String key)` | Yes | |
| `clear()` | Yes | Prefix-based filtering (`flutter.` prefix by default) |

### SharedPreferencesAsync

All standard get/set methods are supported: `String`, `int`, `double`, `bool`, `List<String>`.

| API | Supported | Notes |
|-----|-----------|-------|
| `clear(ClearPreferencesParameters)` | Yes | |
| `getKeys(GetPreferencesParameters)` | Yes | |
| `getPreferences(GetPreferencesParameters)` | Yes | |

## webOS-Specific Behavior

- Data is persisted via a platform method channel using JSON serialization.
- The legacy API includes prefix-based filtering (`flutter.` prefix by default) consistent with the upstream behavior.
