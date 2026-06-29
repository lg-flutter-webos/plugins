import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';
import 'image_texture_method_channel.dart';

abstract class ImageTexturePlatform extends PlatformInterface {
  ImageTexturePlatform() : super(token: _token);

  static final Object _token = Object();

  static ImageTexturePlatform _instance = ImageTextureMethodChannel();

  static ImageTexturePlatform get instance => _instance;

  static set instance(ImageTexturePlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<void> init(int maxCacheSizeBytes) {
    throw UnimplementedError('init() has not been implemented.');
  }

  Future<void> clearCache() {
    throw UnimplementedError('clearCache() has not been implemented.');
  }

  Future<int?> create(DataSource dataSource) {
    throw UnimplementedError('create() has not been implemented.');
  }

  Future<void> dispose(int textureId) {
    throw UnimplementedError('dispose() has not been implemented.');
  }

  Future<void> activate(int textureId) {
    throw UnimplementedError('activate() has not been implemented.');
  }

  Future<void> deactivate(int textureId) {
    throw UnimplementedError('deactivate() has not been implemented.');
  }

  Future<List<int>?> getSize(int textureId) {
    throw UnimplementedError('getSize() has not been implemented.');
  }

  Future<void> play(int textureId) {
    throw UnimplementedError('play() has not been implemented.');
  }

  Future<void> stop(int textureId) {
    throw UnimplementedError('stop() has not been implemented.');
  }

  Stream<ImageEvent> imageEventsFor(int textureId) {
    throw UnimplementedError('imageEventsFor() has not been implemented.');
  }

  Widget buildView(int textureId) {
    throw UnimplementedError('buildView() has not been implemented.');
  }

  Future<void> debugPrint(String msg) {
    throw UnimplementedError('debugPrint() has not been implemented.');
  }
}

class DataSource {
  DataSource({
    required this.sourceType,
    required this.cacheWidth,
    required this.cacheHeight,
    this.uri,
    this.bytes,
  });

  final DataSourceType sourceType;
  final int cacheWidth;
  final int cacheHeight;
  final String? uri;
  final Uint8List? bytes;
}

enum DataSourceType {
  asset,
  network,
  file,
  memory,
  unknown,
}

@immutable
class ImageEvent {
  const ImageEvent({
    required this.eventType,
    this.size,
    this.errorMsg,
  });

  final ImageEventType eventType;
  final Size? size;
  final String? errorMsg;

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        other is ImageEvent &&
            eventType == other.eventType &&
            size == other.size;
  }

  @override
  int get hashCode => Object.hash(
        eventType,
        size,
      );
}

enum ImageEventType {
  initialized,
  error,
  frame,
  completed,
  unknown,
}
