# API Reference — firebase_functions_webos

> **Note:** For standard API documentation, see [`cloud_functions` on pub.dev](https://pub.dev/packages/cloud_functions).
> This document covers webOS-specific behavior only.

## Features

- HTTPS callable function invocation
- Timeout configuration per callable
- Firebase Functions emulator support
- Multi-region support

## Overview

This plugin provides the webOS platform implementation of `cloud_functions`.
It implements `FirebaseFunctionsPlatform` and uses Pigeon-generated platform channels
to communicate with the Firebase C++ Functions SDK.

## Supported APIs

| API | Supported | Notes |
|-----|-----------|-------|
| `httpsCallable()` | Yes | Creates callable reference with options |
| `HttpsCallable.call()` | Yes | Invokes function with optional parameters |
| `HttpsCallable.stream()` | No | Throws `UnimplementedError` |
| `useFunctionsEmulator()` | Yes | |

## webOS-Specific Behavior

- **Region support**: Functions are scoped to a region string (e.g., `us-central1`).
  The region is passed to the C++ SDK for server routing.
- **Timeout**: Configured via `HttpsCallableOptions.timeout` and passed to the
  native layer in milliseconds.
- **Streaming not supported**: `stream()` throws `UnimplementedError`.
  Only request/response invocation via `call()` is available.
- **Error pass-through**: Firebase C++ SDK error codes are passed directly
  as `FirebaseFunctionsException` without modification.
