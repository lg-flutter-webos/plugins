# API Reference — audioplayers_soloud

> **Note:** For standard API documentation, see [audioplayers on pub.dev](https://pub.dev/documentation/audioplayers/latest/).
> This document covers webOS-specific behavior only.

## Features

- Software-based audio playback using the SoLoud audio engine
- Supports simultaneous multi-channel audio playback (up to 32 voices)
- Playback controls: play, pause, stop, seek, volume, balance, playback rate
- Event streams for state, duration, position, and seek completion
- Automatic pause/resume based on app visibility

## Overview

This plugin provides an alternative webOS platform implementation of `audioplayers` using the SoLoud audio engine. Unlike the default `audioplayers` webOS plugin (which uses uMediaClient), this package supports **simultaneous multi-channel audio playback**. Users interact with the standard `audioplayers` API — this package must be explicitly added to `pubspec.yaml` alongside `audioplayers`.

## Supported APIs

### AudioPlayer

Standard playback controls (`play`, `pause`, `stop`, `resume`, `release`, `seek`, `dispose`) and property accessors (`setVolume`, `setPlaybackRate`, `setBalance`, `getDuration`, `getCurrentPosition`) are all supported.

| API | Supported | Notes |
|-----|-----------|-------|
| `setReleaseMode(ReleaseMode mode)` | Yes | `release`, `loop`, `stop` |
| `setPlayerMode(PlayerMode mode)` | Yes | `lowLatency` mode supported |
| `setSource(Source source)` | Yes | URL, asset, bytes |
| `setAudioContext(AudioContext context)` | No | Platform-specific features not applicable |

### Event Streams

All event streams are supported. `onPositionChanged` updates at ~200ms intervals.

## Supported Audio Formats

| Format | Supported |
|--------|-----------|
| WAV | Yes |
| MP3 | Yes |
| OGG Vorbis | Yes |
| FLAC | Yes |

## webOS-Specific Behavior

- Uses the **SoLoud** software audio engine instead of uMediaClient.
- Supports **simultaneous multi-channel playback** (up to 32 voices), unlike the default `audioplayers` webOS plugin.
- Maximum 31 player instances. Exceeding this limit causes the oldest player to error out.
- The plugin monitors app visibility. When the app goes to background, audio is automatically paused unless `enableBackgroundRun` is set to `true` in `appinfo.json`.
- Position updates are throttled to ~200ms intervals.
- Audio output is configured at 44.1 kHz stereo with 2048 sample buffer.
