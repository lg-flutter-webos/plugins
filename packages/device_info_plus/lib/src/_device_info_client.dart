import 'package:flutter/services.dart';

const _channel = MethodChannel('webos_plugin');

Future<Map<String, dynamic>> fetchDeviceInfo() async {
  final result = await _channel.invokeMapMethod<String, dynamic>(
    'device_info/getDeviceInfo',
  );
  return result ?? const {};
}
