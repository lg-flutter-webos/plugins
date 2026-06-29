import 'ls2_service_platform_interface.dart';
import 'service_channel.dart';
import 'webos_service_channel.dart';

class _CallInfo {
  const _CallInfo(this.uri, {this.payload = const {}});

  final String uri;
  final dynamic payload;

  @override
  bool operator ==(Object other) =>
      other is _CallInfo && other.hashCode == hashCode;

  @override
  int get hashCode => _hash(payload.hashCode);

  int _hash(int opt) => '$uri$opt'.hashCode;

  Map<String, dynamic> toJSON() {
    if (payload is! Map<String, dynamic> &&
        payload is! List<dynamic> &&
        !(payload is Map && payload.isEmpty)) {
      throw Exception('Invalid payload type exception.');
    }
    return <String, dynamic>{
      'uri': uri,
      'payload': payload,
      'hashCode': _hash(payload.hashCode),
      'optHashCode': payload.hashCode,
      'serviceName': '',
    };
  }
}

class LS2ServiceChannel {
  static void registerWith() {
    LS2ServicePlatform.instance = WebOSServiceChannel();
  }
}

class WebOSServiceBridge {
  WebOSServiceBridge(String uri, {Map<String, dynamic> payload = const {}})
      : _callInfo = _CallInfo(uri, payload: payload);

  final _CallInfo _callInfo;

  static Future<Map<String, dynamic>?> callOneReply(
    String uri, {
    Map<String, dynamic> payload = const {},
  }) =>
      ServiceChannel().request(
        'callOneReply',
        _CallInfo(uri, payload: payload).toJSON(),
      );

  Stream<Map<String, dynamic>> subscribe() {
    ServiceChannel().request('call', _callInfo.toJSON());
    return ServiceChannel().streamFor(_callInfo.hashCode);
  }

  Future<Map<String, dynamic>?> cancel() =>
      ServiceChannel().request('cancel', _callInfo.toJSON());
}
