import 'dart:ui';

import 'src/display_method_channel.dart';
import 'src/display_platform_interface.dart';
import 'src/event_manager.dart';

const String _kOrientationChangedMethod = 'orientationChanged';

const Set<int> _kRotationDegrees = {0, 90, 180, 270};

class WebOSDisplay {
  WebOSDisplay()
      : _events = WebOSEventManager(DisplayMethodChannel.channel,
            idPrefix: 'DId');

  final WebOSEventManager _events;

  Future<int?> getRotationDegree() => DisplayPlatform.instance.getRotationDegree();

  Future<Size?> getBounds() => DisplayPlatform.instance.getBounds();

  Future<void> setVisible(bool visible) => visible
      ? DisplayPlatform.instance.visible()
      : DisplayPlatform.instance.invisible();

  Future<void> addOrientationListener(void Function(int degree) handler) {
    return _events.bind<int>(
      method: _kOrientationChangedMethod,
      parse: _parseOrientation,
      handler: handler,
    );
  }

  Future<void> removeOrientationListener() =>
      _events.unbind(_kOrientationChangedMethod);

  static void registerWith() {
    DisplayPlatform.instance = DisplayMethodChannel();
  }
}

int _parseOrientation(dynamic e) {
  if (e is! int) return 0;
  if (!_kRotationDegrees.contains(e)) return 0;
  return e;
}
