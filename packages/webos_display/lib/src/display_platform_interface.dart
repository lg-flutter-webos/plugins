import 'dart:ui';

import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'display_method_channel.dart';

abstract class DisplayPlatform extends PlatformInterface {
  DisplayPlatform() : super(token: _token);

  static final Object _token = Object();

  static DisplayPlatform _instance = DisplayMethodChannel();

  static DisplayPlatform get instance => _instance;

  static set instance(DisplayPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<int?> getRotationDegree() {
    throw UnimplementedError('getRotationDegree() has not been implemented.');
  }

  Future<Size?> getBounds() {
    throw UnimplementedError('getBounds() has not been implemented.');
  }

  Future<void> visible() {
    throw UnimplementedError('visible() has not been implemented.');
  }

  Future<void> invisible() {
    throw UnimplementedError('invisible() has not been implemented.');
  }
}
