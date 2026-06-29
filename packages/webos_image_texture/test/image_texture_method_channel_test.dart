import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:webos_image_texture/image_texture_method_channel.dart';

void main() {
  ImageTextureMethodChannel platform = ImageTextureMethodChannel();
  const MethodChannel channel = MethodChannel('image_texture');

  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (MethodCall methodCall) async {
      return '42';
    });
  });

  tearDown(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, null);
  });

  test('runtimeType', () {
    expect(platform.runtimeType, '$ImageTextureMethodChannel');
  });
}
