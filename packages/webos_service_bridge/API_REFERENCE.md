# API Reference — webos_service_bridge

## Features

- Luna-Service2 IPC: single request (`callOneReply`) and streaming subscriptions (`subscribe`)
- Instance-based subscriptions with response streams filtered per request
- Cancel active subscriptions by instance

## Luna Service URI format

All URIs follow this pattern:

```
luna://<service-name>/<method>
```

Example URIs:

| URI | Description |
|-----|-------------|
| `luna://com.webos.settingsservice/getSystemSettings` | Query system settings |
| `luna://com.palm.systemservice/time/getSystemTime` | Get system time |
| `luna://com.webos.service.connectionmanager/getStatus` | Get network connection status |

## API Overview

### Classes

- **WebOSServiceBridge** — Interface to webOS Luna-Service2 for service calls and subscriptions.

---

## Classes

### WebOSServiceBridge

Provides an interface to webOS Luna-Service2 for making service calls and subscriptions.

#### Constructor

```dart
WebOSServiceBridge(String uri, {Map<String, dynamic> payload = const {}})
```

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| uri | `String` | Yes | The Luna service URI (e.g., `'luna://com.webos.service.connectionmanager/getStatus'`) |
| payload | `Map<String, dynamic>` | No | The request payload (default: `{}`) |

> For subscriptions, include `'subscribe': true` in the payload.

#### Methods

##### subscribe

```dart
Stream<Map<String, dynamic>> subscribe()
```

Starts a subscription to the service and returns a filtered stream of responses.

Each `WebOSServiceBridge` instance maintains its own independent stream, so multiple simultaneous subscriptions are supported.

**Returns:** `Stream<Map<String, dynamic>>` — Stream of service responses for this instance.

> Always call `cancel()` when the subscription is no longer needed to release resources.

##### cancel

```dart
Future<Map<String, dynamic>?> cancel()
```

Cancels the current subscription for this instance.

**Returns:** `Future<Map<String, dynamic>?>` — The cancellation response.

#### Static Methods

##### callOneReply

```dart
static Future<Map<String, dynamic>?> callOneReply(String uri, {Map<String, dynamic> payload = const {}})
```

Makes a single Luna service call and returns one response. Use this for one-shot requests that do not require a subscription.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| uri | `String` | Yes | The Luna service URI |
| payload | `Map<String, dynamic>` | No | The request payload (default: `{}`) |

**Returns:** `Future<Map<String, dynamic>?>` — The service response.

---

## Notes

- The `payload` must be a `Map<String, dynamic>` or an empty map. Other types will throw an exception at runtime.
- For subscription calls, pass `'subscribe': true` in the payload, otherwise the service may return only one response and close.
- Multiple `WebOSServiceBridge` instances can subscribe to the same URI simultaneously; each receives its own filtered stream.