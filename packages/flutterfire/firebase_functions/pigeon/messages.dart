import 'package:pigeon/pigeon.dart';

@ConfigurePigeon(
  PigeonOptions(
    dartOut: 'lib/messages.g.dart',
    cppHeaderOut: 'webos/messages.g.h',
    cppSourceOut: 'webos/messages.g.cc',
    cppOptions: CppOptions(
      namespace: 'firebase_functions_webos',
    ),
    dartPackageName: 'firebase_functions_webos',
  ),
)

class HttpsCallableRequest {
  String? appName;
  String region;
  String? origin;
  String functionName;
  Object? parameters;
  int timeoutMs;
}

class HttpsCallableResponse {
  Object? data;
}

// ---------------------------------------------------------------------------
// Flutter → C++
// ---------------------------------------------------------------------------

@HostApi()
abstract class FirebaseFunctionsHostApi {
  @async
  HttpsCallableResponse callFunction(
    HttpsCallableRequest request,
  );

  @async
  void useFunctionsEmulator(
    String appName,
    String region,
    String host,
    int port,
  );
}