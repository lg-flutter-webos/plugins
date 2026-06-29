# API Reference — webos_display

## Features

- Queries panel rotation in degrees (`0`, `90`, `180`, `270`)
- Queries panel bounds as a `Size` from Wayland's `wl_output`
- Shows and hides the panel via the existing webOS `visible` / `invisible` methods
- Streams orientation-change events with a single handler per instance

## API Overview

### Classes

- **WebOSDisplay** — Entry point for querying webOS display state and listening for orientation changes.

---

## Classes

### WebOSDisplay

Entry point for querying webOS display state and listening for orientation changes.

#### Constructor

```dart
WebOSDisplay()
```

#### Methods

##### addOrientationListener

```dart
Future<void> addOrientationListener(void Function(int degree) handler)
```

Subscribes to orientation-change events. The handler receives one of `0`, `90`, `180`, or `270`. Subscribing twice on the same instance swaps the handler — there is no extra `connect` round-trip and no duplicate streams.

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| handler | `void Function(int degree)` | Yes | Callback invoked with each rotation degree event. |

**Returns:** `Future<void>` — Completes once the native subscription is bound.

##### getBounds

```dart
Future<Size?> getBounds()
```

Returns the panel bounds reported by Wayland's `wl_output`.

**Returns:** `Size?` — The panel bounds, or `null` if the native side has not produced a value yet or the reply payload was malformed.

##### getRotationDegree

```dart
Future<int?> getRotationDegree()
```

Returns the current panel rotation in degrees.

**Returns:** `int?` — One of `0`, `90`, `180`, `270`, or `null` if the native side has not produced a value yet or the reply payload was malformed.

##### removeOrientationListener

```dart
Future<void> removeOrientationListener()
```

Unsubscribes the current orientation handler.

**Returns:** `Future<void>` — Completes once the native subscription is released.

##### setVisible

```dart
Future<void> setVisible(bool visible)
```

Shows the panel when `visible` is `true`, hides it when `false`. Dispatches to the existing `visible` / `invisible` 

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| visible | `bool` | Yes | `true` to show the panel, `false` to hide it. |

**Returns:** `Future<void>` — Completes once the native command has been dispatched.
