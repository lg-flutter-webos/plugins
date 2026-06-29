# API Reference — audioplayers_webos

> **Note:** For standard API documentation, see [audioplayers on pub.dev](https://pub.dev/documentation/audioplayers/latest/).
> This document covers webOS-specific behavior only.

## Features

- Audio playback from URL, asset, and byte data sources
- Playback controls: play, pause, stop, seek, playback rate
- Event streams for state, duration, position, and seek completion
- Automatic pause/resume based on app visibility
- Loop and release mode support

## Overview

This plugin provides the webOS platform implementation of `audioplayers`. Users interact with the standard `audioplayers` API — this package is registered automatically and requires no direct usage.

## Supported APIs

### AudioPlayer

Standard playback controls (`play`, `pause`, `stop`, `resume`, `release`, `seek`, `dispose`) and property accessors (`setPlaybackRate`, `setPlayerMode`, `getDuration`, `getCurrentPosition`) are all supported.

| API | Supported | Notes |
|-----|-----------|-------|
| `setVolume(double volume)` | No | |
| `setBalance(double balance)` | No | |
| `setReleaseMode(ReleaseMode mode)` | Yes | `release`, `loop`, `stop` |
| `setSource(Source source)` | Yes | URL, asset, bytes |
| `setAudioContext(AudioContext context)` | No | Platform-specific features not applicable |

### Event Streams

All event streams are supported. `onPositionChanged` updates at ~200ms intervals.

## webOS-Specific Behavior

- Uses **uMediaClient** (webOS media framework) as the native backend.
- Only one audio can play at a time. For simultaneous playback, use `audioplayers_soloud` instead.
- Maximum 5 player instances. Exceeding this limit causes the oldest player to error out.
- The plugin monitors app visibility. When the app goes to background, audio is automatically paused unless `enableBackgroundRun` is set to `true` in `appinfo.json`.
- Position updates are throttled to ~200ms intervals.
- Byte data sources are written to a temporary file before playback.
