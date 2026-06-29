import 'dart:async';

import 'package:connectivity_plus_platform_interface/connectivity_plus_platform_interface.dart';
import 'package:flutter/services.dart';

const _channel = EventChannel('webos_plugin/connection_manager/status/stream');

class ConnectivityClient {
  List<ConnectivityResult> _last = const [ConnectivityResult.none];
  bool _hasValue = false;

  late final Stream<List<ConnectivityResult>> _stream = _channel
      .receiveBroadcastStream()
      .map(_parseEvent)
      .map((v) {
        _last = v;
        _hasValue = true;
        return v;
      })
      .asBroadcastStream();

  Stream<List<ConnectivityResult>> get statusStream => _stream;

  Future<List<ConnectivityResult>> current() async {
    if (_hasValue) return _last;
    final completer = Completer<List<ConnectivityResult>>();
    late StreamSubscription<List<ConnectivityResult>> sub;
    sub = statusStream.listen((event) {
      if (!completer.isCompleted) completer.complete(event);
      sub.cancel();
    });
    return completer.future;
  }
}

List<ConnectivityResult> _parseEvent(dynamic event) {
  if (event is! List || event.isEmpty) return const [ConnectivityResult.none];
  return event
      .map((t) => ConnectivityResult.values.byName(t as String))
      .toList();
}
