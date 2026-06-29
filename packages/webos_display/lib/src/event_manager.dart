import 'dart:async';

import 'package:flutter/services.dart';

class WebOSEventManager {
  WebOSEventManager(this._channel, {required this.idPrefix});

  final MethodChannel _channel;
  final String idPrefix;

  static int _seq = 0;
  late final String instanceId = '$idPrefix${(++_seq).toRadixString(16)}';

  final _bindings = <String, _Binding>{};
  final _pending = <String, Future<void>>{};

  Future<void> bind<T>({
    required String method,
    required T Function(dynamic) parse,
    required void Function(T) handler,
  }) {
    return _serialize(method, () async {
      final existing = _bindings[method];
      if (existing != null) {
        existing.handler = handler;
        return;
      }
      await _channel.invokeMethod<void>('connect', {
        'method': method,
        'instanceId': instanceId,
      });
      final sub = EventChannel('${_channel.name}/events/$method$instanceId')
          .receiveBroadcastStream()
          .listen((dynamic e) {
        final b = _bindings[method];
        if (b != null) (b.handler as void Function(T))(parse(e));
      });
      _bindings[method] = _Binding(handler, sub);
    });
  }

  Future<void> unbind(String method) {
    return _serialize(method, () async {
      final b = _bindings.remove(method);
      if (b == null) return;
      await b.subscription.cancel();
      await _channel.invokeMethod<void>('disconnect', {
        'method': method,
        'instanceId': instanceId,
      });
    });
  }

  Future<void> _serialize(String method, Future<void> Function() op) async {
    while (_pending[method] != null) {
      await _pending[method];
    }
    final completer = Completer<void>();
    _pending[method] = completer.future;
    try {
      await op();
    } finally {
      _pending.remove(method);
      completer.complete();
    }
  }
}

class _Binding {
  _Binding(this.handler, this.subscription);
  Object handler;
  final StreamSubscription<dynamic> subscription;
}
