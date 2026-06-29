# API Reference — sqflite_webos

> **Note:** For standard API documentation, see [sqflite on pub.dev](https://pub.dev/documentation/sqflite/latest/).
> This document covers webOS-specific behavior only.

## Features

- Full SQLite database support via native C++ implementation
- Standard CRUD operations with parameter binding
- Batch operations with optional continue-on-error
- In-memory and file-based databases

## Overview

This plugin provides the webOS platform implementation of `sqflite`. Users interact with the standard `sqflite` API — this package must be explicitly added to `pubspec.yaml` alongside `sqflite`.

## Supported APIs

Standard database operations (`closeDatabase`, `deleteDatabase`, `databaseExists`, `execute`, `rawQuery`) and in-memory databases (`:memory:`) are all supported.

| API | Supported | Notes |
|-----|-----------|-------|
| `openDatabase(path)` | Yes | Supports `readOnly` and `singleInstance` options |
| `getDatabasesPath()` | Yes | Resolves to `${FLUTTER_APPDATA_HOME}/${FLUTTER_APP_ID}/` |
| `rawInsert(sql)` | Yes | Returns last insert ID |
| `rawUpdate(sql)` | Yes | Returns changed row count |
| `rawDelete(sql)` | Yes | Returns changed row count |
| `batch()` | Yes | Supports `continueOnError` |

## Supported Data Types

| SQLite Type | Dart Type |
|-------------|-----------|
| NULL | `null` |
| INTEGER | `int` |
| REAL | `double` |
| TEXT | `String` |
| BLOB | `Uint8List` |

## webOS-Specific Behavior

- Databases are stored at `${FLUTTER_APPDATA_HOME}/${FLUTTER_APP_ID}/`. Both environment variables must be set, or a `PlatformException` is thrown.
- SQLite busy timeout is set to 2500ms.
- Database access is thread-safe via mutex protection.
