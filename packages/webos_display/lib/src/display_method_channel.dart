import 'package:flutter/services.dart';

import 'display_platform_interface.dart';

class DisplayMethodChannel extends DisplayPlatform {
  static const MethodChannel channel = MethodChannel('webos/system');

  @override
  Future<int?> getRotationDegree() async {
    final degree = await channel.invokeMethod<int>('rotationDegree', {});
    if (degree == null) return null;
    if (degree != 0 && degree != 90 && degree != 180 && degree != 270) {
      return null;
    }
    return degree;
  }

  @override
  Future<Size?> getBounds() async {
    final pair = await channel.invokeMethod<List<dynamic>>('displayBounds', {});
    if (pair == null || pair.length < 2) return null;
    final w = pair[0];
    final h = pair[1];
    if (w is! num || h is! num) return null;
    if (w <= 0 || h <= 0) return null;
    return Size(w.toDouble(), h.toDouble());
  }

  @override
  Future<void> visible() => channel.invokeMethod<void>('visible', {});

  @override
  Future<void> invisible() => channel.invokeMethod<void>('invisible', {});
}
