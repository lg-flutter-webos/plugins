import 'package:flutter/services.dart';

import '../webos_app_manager.dart' show AppLoadStatus;

class AppManagerMethodChannel {
  static const MethodChannel channel = MethodChannel('webos_plugin');

  Future<AppLoadStatus?> getAppLoadStatus(String appId) async {
    final reply = await channel
        .invokeMethod<Map<dynamic, dynamic>>('app_manager/getAppLoadStatus', {'appId': appId});
    if (reply == null) return null;
    return AppLoadStatus.fromMap(reply);
  }
}
