import 'dart:async';
import 'package:flutter/services.dart';
import 'package:flutter_keyboard_visibility_platform_interface/flutter_keyboard_visibility_platform_interface.dart';

/// The webOS implementation of the [FlutterKeyboardVisibilityPlatform] of the
/// FlutterKeyboardVisibility plugin.
class FlutterKeyboardVisibilityPluginWebOS
    extends FlutterKeyboardVisibilityPlatform {
  /// Factory method that initializes the FlutterKeyboardVisibility plugin
  /// platform with an instance of the plugin for WebOS
  static void registerWith() {
    FlutterKeyboardVisibilityPlatform.instance =
        FlutterKeyboardVisibilityPluginWebOS();
  }

  static final _onChangeController = StreamController<bool>();
  static final _onChange = _onChangeController.stream.asBroadcastStream();
  static bool _isInitialized = false;

  /// Emits changes to keyboard visibility from the platform.
  static bool get isVisible => _isVisible;
  static bool _isVisible = false;

  static void _updateValue(bool newValue) {
    print('visibility : $newValue');
    if (newValue == _isVisible) {
      return;
    }

    _isVisible = newValue;
    _onChangeController.add(newValue);
  }

  @override

  /// Emits true every time the keyboard is shown, and false every time the
  /// keyboard is dismissed.
  Stream<bool> get onChange {
    if (!_isInitialized) {
      _addKeyboardStatusListener(_updateValue);

      _isInitialized = true;
    }
    return _onChange;
  }

  //// For WebOS....
  final _webos_method = 'keyboardStatus';
  final _webos_instanceId = 'flutter_keyboard_visibility_webos';
  // ignore: unused_field
  late dynamic _webos_eventSubscription;
  late Function(bool newValue) _webos_handler;

  void _addKeyboardStatusListener(Function(bool newValue) handler) async {
    final channel = const MethodChannel('webos/system');

    await channel.invokeMethod<void>('connect', {
      'method': _webos_method,
      'instanceId': _webos_instanceId,
    });

    _webos_handler = handler;
    _webos_eventSubscription = _keyboardStatusEvent().listen((_) {});
  }

  Stream<void> _keyboardStatusEvent() {
    final eventChannel = EventChannel(
        'webos/system/events/' + _webos_method + _webos_instanceId);

    return eventChannel.receiveBroadcastStream().map((dynamic event) {
      var args = event is bool ? event : false;
      _webos_handler(args);
    });
  }
}
