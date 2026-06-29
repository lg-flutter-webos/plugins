import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'image_texture_platform_interface.dart';

class ImageTextureMethodChannel extends ImageTexturePlatform {
  final _channel = const MethodChannel('webos/image_texture');

  @override
  Future<void> init(int maxCacheSizeBytes) async {
    _channel.invokeMethod<void>(
        'initialize', {'maximumCacheBytes': maxCacheSizeBytes});
  }

  @override
  Future<void> clearCache() async {
    _channel.invokeMethod<void>('clearCache');
  }

  @override
  Future<int?> create(DataSource dataSource) async {
    int? textureId;
    if (dataSource.sourceType == DataSourceType.memory) {
      textureId = await _channel.invokeMethod<int>(
        'create',
        {
          'sourceType': dataSource.sourceType.toString(),
          'cacheWidth': dataSource.cacheWidth,
          'cacheHeight': dataSource.cacheHeight,
          'bytes': dataSource.bytes,
        },
      );
    } else {
      textureId = await _channel.invokeMethod<int>(
        'create',
        {
          'sourceType': dataSource.sourceType.toString(),
          'cacheWidth': dataSource.cacheWidth,
          'cacheHeight': dataSource.cacheHeight,
          'uri': dataSource.uri.toString(),
        },
      );
    }
    return textureId;
  }

  @override
  Future<void> dispose(int textureId) async {
    _channel.invokeMethod<void>('dispose', {'textureId': textureId});
  }

  @override
  Future<void> activate(int textureId) async {
    _channel.invokeMethod<void>('activate', {'textureId': textureId});
  }

  @override
  Future<void> deactivate(int textureId) async {
    _channel.invokeMethod<void>('deactivate', {'textureId': textureId});
  }

  @override
  Future<List<int>?> getSize(int textureId) async {
    return await _channel.invokeMethod<List<int>>('getSize', {'textureId': textureId});
  }

  @override
  Future<void> play(int textureId) async {
    _channel.invokeMethod<void>('play', {'textureId': textureId});
  }

  @override
  Future<void> stop(int textureId) async {
    _channel.invokeMethod<void>('stop', {'textureId': textureId});
  }

  @override
  Stream<ImageEvent> imageEventsFor(int textureId) {
    final eventChannel = EventChannel('webos/image_texture/event$textureId');
    Stream<ImageEvent> stream =
        eventChannel.receiveBroadcastStream().map((dynamic event) {
      var map = event as Map<dynamic, dynamic>;
      var eventType = (map['event'] as String?) ?? 'invalid';

      switch (eventType) {
        case 'initialized':
          return ImageEvent(
            eventType: ImageEventType.initialized,
            size: Size(map['width']?.toDouble() ?? 0.0,
                map['height']?.toDouble() ?? 0.0),
          );

        case 'error':
          return ImageEvent(
            eventType: ImageEventType.error,
            errorMsg: map['errorMsg']?.toString() ?? 'image_texture error',
          );

        case 'frame':
          return const ImageEvent(eventType: ImageEventType.frame);

        case 'completed':
          return const ImageEvent(eventType: ImageEventType.completed);

        default:
          return const ImageEvent(eventType: ImageEventType.unknown);
      }
    });

    return stream;
  }

  @override
  Widget buildView(int textureId) {
    return Texture(textureId: textureId);
  }

  @override
  Future<void> debugPrint(String msg) async {
    _channel.invokeMethod<void>('log', {'msg': msg});
  }
}
