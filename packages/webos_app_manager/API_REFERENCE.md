# API Reference — webos_app_manager

## Features

- Checks whether a webOS application is currently loaded

## API Overview

### Classes

- **WebOSAppManager** — Entry point for checking app load status.
- **AppLoadStatus** — Describes whether an app is currently loaded.

---

## Classes

### WebOSAppManager

Entry point for checking app load status.

#### Constructor

```dart
WebOSAppManager()
```

#### Methods

##### getAppLoadStatus

```dart
Future<AppLoadStatus?> getAppLoadStatus(String appId)
```

Returns the load status of the specified application.

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| appId | `String` | Yes | The application identifier to query. |

**Returns:** `AppLoadStatus?` — The load status of the application, or `null` if the native side has not responded or the payload was malformed.

---

### AppLoadStatus

Describes whether an app is currently loaded.

#### Constructor

```dart
const AppLoadStatus({required bool exist})
```

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| exist | `bool` | Yes | Whether the specified application is currently loaded. |

#### Properties

| Property | Type | Description |
|----------|------|-------------|
| exist | `bool` | Whether the specified application is currently loaded. |
