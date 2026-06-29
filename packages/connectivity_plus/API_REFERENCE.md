# API Reference — connectivity_plus_webos

> **Note:** For standard API documentation, see [connectivity_plus on pub.dev](https://pub.dev/documentation/connectivity_plus/latest/).
> This document covers webOS-specific behavior only.

## Features

- Real-time network connectivity status monitoring
- Support for WiFi, Ethernet, Cellular, and Bluetooth connection types

## Overview

This plugin provides the webOS platform implementation of `connectivity_plus`. Users interact with the standard `connectivity_plus` API — this package must be explicitly added to `pubspec.yaml` alongside `connectivity_plus`.

## Supported APIs

| API | Supported | Notes |
|-----|-----------|-------|
| `checkConnectivity()` | Yes | Returns `List<ConnectivityResult>` |
| `onConnectivityChanged` | Yes | Stream of connectivity changes |

## Supported Connection Types

| ConnectivityResult | Supported | Notes |
|--------------------|-----------|-------|
| `wifi` | Yes | |
| `ethernet` | Yes | Mapped from webOS "wired" interface |
| `mobile` | Yes | Mapped from webOS "cellular" interface |
| `bluetooth` | Yes | |
| `vpn` | No | |
| `none` | Yes | When `isInternetConnectionAvailable` is `false` |
| `other` | Yes | Fallback for unknown connection types |

## webOS-Specific Behavior

- Network status is obtained from the webOS `connectionmanager` Luna service with a subscription for real-time updates.
- Returns multiple connection types simultaneously when multiple interfaces are connected (e.g., both WiFi and Ethernet).
- VPN detection is not supported on webOS.
