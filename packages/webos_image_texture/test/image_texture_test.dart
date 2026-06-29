import 'package:flutter/src/widgets/framework.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';
import 'package:webos_image_texture/image_texture_method_channel.dart';
import 'package:webos_image_texture/image_texture_platform_interface.dart';

class MockImageTexturePlatform
    with MockPlatformInterfaceMixin
    implements ImageTexturePlatform {
  Future<String?> getPlatformVersion() => Future.value('42');

  @override
  Future<void> activate(int textureId) {
    throw UnimplementedError();
  }

  @override
  Widget buildView(int textureId) {
    throw UnimplementedError();
  }

  @override
  Future<void> clearCache() {
    throw UnimplementedError();
  }

  @override
  Future<int?> create(DataSource dataSource) {
    throw UnimplementedError();
  }

  @override
  Future<void> deactivate(int textureId) {
    throw UnimplementedError();
  }

  @override
  Future<void> debugPrint(String msg) {
    throw UnimplementedError();
  }

  @override
  Future<void> dispose(int textureId) {
    throw UnimplementedError();
  }

  @override
  Stream<ImageEvent> imageEventsFor(int textureId) {
    throw UnimplementedError();
  }

  @override
  Stream<ImageEvent> imageVsyncEvents() {
    throw UnimplementedError();
  }

  @override
  Future<void> init(int maxCacheSizeBytes) {
    throw UnimplementedError();
  }

  @override
  Future<void> pause(int textureId) {
    throw UnimplementedError();
  }

  @override
  Future<void> play(int textureId) {
    throw UnimplementedError();
  }

  @override
  Future<void> setLooping(int textureId, bool looping) {
    throw UnimplementedError();
  }

  @override
  Future<void> stop(int textureId) {
    throw UnimplementedError();
  }
  
  @override
  Future<List<int>?> getSize(int textureId) {
    throw UnimplementedError();
  }
}

void main() {
  final ImageTexturePlatform initialPlatform = ImageTexturePlatform.instance;

  test('$ImageTextureMethodChannel is the default instance', () {
    expect(initialPlatform, isInstanceOf<ImageTextureMethodChannel>());
  });

  test('getPlatformVersion', () {
    ImageTextureMethodChannel imageTexturePlugin = ImageTextureMethodChannel();
    MockImageTexturePlatform fakePlatform = MockImageTexturePlatform();
    ImageTexturePlatform.instance = fakePlatform;

    expect(imageTexturePlugin.runtimeType, '$ImageTextureMethodChannel');
  });
}
