import 'package:plugin_platform_interface/plugin_platform_interface.dart';

typedef LS2PayloadType = Map<String, dynamic>;

abstract class LS2ServicePlatform extends PlatformInterface {
  LS2ServicePlatform() : super(token: _token);

  static final Object _token = Object();

  static LS2ServicePlatform? _instance;

  static LS2ServicePlatform get instance {
    assert(
      _instance != null,
      'LS2ServiceChannel.registerWith() must be called before use',
    );
    return _instance!;
  }

  static set instance(LS2ServicePlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<LS2PayloadType?> request(
    String requestName,
    LS2PayloadType payload,
  ) async {
    throw UnimplementedError('request(...) has not been implemented.');
  }

  Stream<LS2PayloadType> listen() {
    throw UnimplementedError('listen(...) has not been implemented.');
  }

  void cancel() {
    throw UnimplementedError('cancel(...) has not been implemented.');
  }
}
