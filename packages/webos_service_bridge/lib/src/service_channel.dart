import 'ls2_service_platform_interface.dart';

class ServiceChannel {
  static final ServiceChannel _instance = ServiceChannel._();

  ServiceChannel._();

  factory ServiceChannel() => _instance;

  late final Stream<Map<String, dynamic>> _events =
      LS2ServicePlatform.instance.listen();

  Future<LS2PayloadType?> request(
    String requestName,
    LS2PayloadType payload,
  ) =>
      LS2ServicePlatform.instance.request(requestName, payload);

  Stream<Map<String, dynamic>> streamFor(int callId) => _events
      .where((e) => (e['_LSCALL_ID'] as int) == callId)
      .map((e) => Map.of(e)..remove('_LSCALL_ID'));
}
