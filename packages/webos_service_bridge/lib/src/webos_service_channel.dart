import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'ls2_service_platform_interface.dart';

class WebOSServiceChannel extends LS2ServicePlatform {
  WebOSServiceChannel() : super() {
    debugPrint('Cancel previous subsciprtion');
    _cancel();
  }

  @override
  Future<LS2PayloadType?> request(
    String requestName,
    LS2PayloadType payload,
  ) async {
    return await platform.invokeMethod<LS2PayloadType?>(requestName, payload);
  }

  @override
  Stream<LS2PayloadType> listen() {
    return event.receiveBroadcastStream().map((dynamic event) => event);
  }

  @override
  void cancel() {
    _cancel();
  }

  void _cancel() async {
    const MethodChannel methodChannel = MethodChannel(
      'webOS/luna-service2/subscribe',
      JSONMethodCodec(),
    );

    try {
      await methodChannel.invokeMethod<void>('cancel', null);
    } catch (exception) {
      debugPrint((exception as PlatformException).message);
    }
  }

  static const MethodChannel platform = MethodChannel(
    'webOS/luna-service2/request',
    JSONMethodCodec(),
  );
  static const EventChannel event = EventChannel(
    'webOS/luna-service2/subscribe',
    JSONMethodCodec(),
  );
}
