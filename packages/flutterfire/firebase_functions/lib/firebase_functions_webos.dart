import 'dart:async';

import 'package:cloud_functions_platform_interface/cloud_functions_platform_interface.dart';
import 'package:firebase_core/firebase_core.dart';

import 'messages.g.dart';

class FirebaseFunctionsWebos extends FirebaseFunctionsPlatform {
  final FirebaseApp? _app;
  final FirebaseFunctionsHostApi _hostApi =
      FirebaseFunctionsHostApi();

  FirebaseApp get app => _app ?? Firebase.app();

  FirebaseFunctionsWebos._empty()
      : _app = null,
        super(null, 'us-central1');

  FirebaseFunctionsWebos({
    FirebaseApp? app,
    required String region,
  })  : _app = app,
        super(app, region);

  // ---------------------------------------------------------------------------
  // REGISTER WITH (NO SINGLETON LOGIC HERE)
  // ---------------------------------------------------------------------------
  static void registerWith() {

    FirebaseFunctionsPlatform.instance =
        FirebaseFunctionsWebos._empty();
  }

  // ---------------------------------------------------------------------------
  // DELEGATE FOR (IMPORTANT)
  // ---------------------------------------------------------------------------
  @override
  FirebaseFunctionsPlatform delegateFor({
    FirebaseApp? app,
    required String region,
  }) {

    return FirebaseFunctionsWebos(
      app: app ?? Firebase.app(),
      region: region,
    );
  }

  // ---------------------------------------------------------------------------
  // HTTPS CALLABLE
  // ---------------------------------------------------------------------------
  @override
  HttpsCallablePlatform httpsCallable(
    String? origin,
    String name,
    HttpsCallableOptions options,
  ) {

    return HttpsCallableWebos(
      this,
      origin,
      name,
      options,
    );
  }

  Future<void> useFunctionsEmulator({
    required String host,
    required int port,
  }) {

    final origin = 'http://$host:$port';

    return _hostApi.useFunctionsEmulator(
      app.name,
      region,
      host,
      port,
    );
  }
}

// ---------------------------------------------------------------------------
// HTTPS CALLABLE IMPLEMENTATION
// ---------------------------------------------------------------------------
class HttpsCallableWebos extends HttpsCallablePlatform {
  final FirebaseFunctionsHostApi _hostApi =
      FirebaseFunctionsHostApi();

  HttpsCallableWebos(
    FirebaseFunctionsPlatform functions,
    String? origin,
    String name,
    HttpsCallableOptions options,
  ) : super(
          functions,
          origin,
          name,
          options,
          null,
        );

  @override
  Future<Object?> call([Object? parameters]) async {

    final response = await _hostApi.callFunction(
      HttpsCallableRequest(
        appName: functions.app?.name,
        region: functions.region,
        origin: origin,
        functionName: name!,
        parameters: parameters,
        timeoutMs: options.timeout.inMilliseconds,
      ),
    );
    return response.data;
  }

  @override
  Stream<dynamic> stream(Object? parameters) {
    throw UnimplementedError('stream() not implemented');
  }
}