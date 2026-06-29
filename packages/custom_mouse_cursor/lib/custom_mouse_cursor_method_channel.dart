import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'custom_mouse_cursor_platform_interface.dart';

/// An implementation of [CustomMouseCursorPlatform] that uses method channels.
class MethodChannelCustomMouseCursor extends CustomMouseCursorPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel('custom_mouse_cursor');

  @override
  Future<String?> getPlatformVersion() async {
    final version = await methodChannel.invokeMethod<String>('getPlatformVersion');
    return version;
  }
}
