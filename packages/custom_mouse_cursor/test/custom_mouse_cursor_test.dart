import 'package:flutter_test/flutter_test.dart';
import 'package:custom_mouse_cursor/custom_mouse_cursor_platform_interface.dart';
import 'package:custom_mouse_cursor/custom_mouse_cursor_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockCustomMouseCursorPlatform
    with MockPlatformInterfaceMixin
    implements CustomMouseCursorPlatform {
  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final CustomMouseCursorPlatform initialPlatform =
      CustomMouseCursorPlatform.instance;

  test('$MethodChannelCustomMouseCursor is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelCustomMouseCursor>());
  });

  test('getPlatformVersion', () async {
    // CustomMouseCursor customMouseCursorPlugin = CustomMouseCursor();
    MockCustomMouseCursorPlatform fakePlatform =
        MockCustomMouseCursorPlatform();
    CustomMouseCursorPlatform.instance = fakePlatform;

    expect(await fakePlatform.getPlatformVersion(), '42');
  });
}
