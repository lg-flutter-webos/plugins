// ignore_for_file: require_trailing_commas
import 'dart:async';

import 'package:firebase_core_platform_interface/firebase_core_platform_interface.dart';
import 'firebase_app_webos.dart';
import 'src/messages.g.dart';

class FirebaseCoreWebos extends FirebasePlatform {
  final FirebaseCoreHostApi _api = FirebaseCoreHostApi();
  final Map<String, FirebaseAppPlatform> _appCache = {};

  static void registerWith() {
    FirebasePlatform.instance = FirebaseCoreWebos();
  }

  FirebaseCoreWebos() : super() {}

  @override
  Future<FirebaseAppPlatform> initializeApp({
    String? name,
    FirebaseOptions? options,
  }) async {
    if (options == null) {
      throw ArgumentError('FirebaseOptions are required on WebOS.');
    }

    final appName = name ?? defaultFirebaseAppName;

    if (_appCache.containsKey(appName)) {
      return _appCache[appName]!;
    }

    final pigeonOptions = FirebaseOptionsPigeon(
      apiKey: options.apiKey,
      appId: options.appId,
      projectId: options.projectId,
      messagingSenderId: options.messagingSenderId,
      storageBucket: options.storageBucket,
      databaseUrl: options.databaseURL,
    );

    try {
      await _api.initializeApp(appName, pigeonOptions).timeout(
        Duration(seconds: 30),
        onTimeout: () {
          throw TimeoutException("Firebase initializeApp timed out after 30 seconds");
        },
      );
    } on TimeoutException catch (e) {
      rethrow;
    } catch (e) {
      rethrow;
    }

    final appPlatform = FirebaseAppWebos(appName, options, _api, _appCache);
    _appCache[appName] = appPlatform;

    return appPlatform;
  }

  @override
  FirebaseAppPlatform app([String name = defaultFirebaseAppName]) {
    final cached = _appCache[name];

    if (cached == null) {
      throw FirebaseException(
        plugin: 'core',
        code: 'no-app',
        message:
            'No Firebase App \'$name\' has been created. '
            'Call Firebase.initializeApp() first.',
      );
    }

    return cached;
  }

  @override
  List<FirebaseAppPlatform> get apps => _appCache.values.toList();

  @override
  Future<void> delete(String name) async {
    await _api.deleteApp(name);
    _appCache.remove(name);
  }

}