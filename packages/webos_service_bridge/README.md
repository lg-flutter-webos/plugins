# webos_service_bridge

Provides interface for luna-service2

## Supported platforms

This plugin is supported on webOS 26 media or above.

## Usage

**pubspec.yaml:**

```yaml
dependencies:
  webos_service_bridge:
    git:
      url: https://github.com/lg-flutter-webos/plugins.git
      path: packages/webos_service_bridge
      ref: main
```

**Import:**

```dart
import 'package:webos_service_bridge/webos_service_bridge.dart';
```

### Single request

Use `callOneReply` to make a one-shot Luna service call and get a single response.

```dart
final response = await WebOSServiceBridge.callOneReply(
  'luna://com.webos.settingsservice/getSystemSettings',
  payload: {'keys': ['localeInfo'], 'subscribe': false},
);
print(response);
```

### Subscription

Use `WebOSServiceBridge` instance with `subscribe()` to receive a continuous stream of responses.
Always call `cancel()` when the subscription is no longer needed.

```dart
final bridge = WebOSServiceBridge(
  'luna://com.webos.service.connectionmanager/getStatus',
  payload: {'subscribe': true},
);

final subscription = bridge.subscribe().listen((data) {
  print(data);
});

// When done:
await bridge.cancel();
await subscription.cancel();
```

## Required configurations

Add the Luna ACGs required by the services you intend to call to your `appinfo.json`.

```json
{
  "requiredACG": [""]
}
```

> The required ACG values depend on which Luna service URIs your app calls.
> Refer to the [ACG Guide](https://dv.webostv.developer.lge.com/develop/guides/acg-guide) for the ACG of each Luna service.

## Example

See the [example application](example/lib/main.dart) for a complete usage demonstration.

## API Reference

See the [API Reference](API_REFERENCE.md) for detailed documentation of all classes, methods, and parameters.