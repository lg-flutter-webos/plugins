import 'package:flutter/services.dart';

const _channel = MethodChannel('webos_plugin');

class LauncherClient {
  bool _busy = false;

  Future<bool> launch(String url) async {
    if (_busy) return false;
    if (!url.startsWith('http://') && !url.startsWith('https://')) return false;

    _busy = true;
    try {
      final result = await _channel.invokeMethod<bool>(
        'app_manager/launch',
        <String, dynamic>{
          'id': 'com.webos.app.browser',
          'params': <String, dynamic>{'target': url},
        },
      );
      return result ?? false;
    } finally {
      _busy = false;
    }
  }
}
