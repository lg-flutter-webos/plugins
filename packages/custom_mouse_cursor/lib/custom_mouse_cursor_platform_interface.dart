import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'custom_mouse_cursor_method_channel.dart';

abstract class CustomMouseCursorPlatform extends PlatformInterface {
  /// Constructs a CustomMouseCursorPlatform.
  CustomMouseCursorPlatform() : super(token: _token);

  static final Object _token = Object();

  static CustomMouseCursorPlatform _instance = MethodChannelCustomMouseCursor();

  /// The default instance of [CustomMouseCursorPlatform] to use.
  ///
  /// Defaults to [MethodChannelCustomMouseCursor].
  static CustomMouseCursorPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [CustomMouseCursorPlatform] when
  /// they register themselves.
  static set instance(CustomMouseCursorPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<String?> getPlatformVersion() {
    throw UnimplementedError('platformVersion() has not been implemented.');
  }
}
