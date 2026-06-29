import 'package:pigeon/pigeon.dart';

@ConfigurePigeon(PigeonOptions(
  dartOut: 'lib/src/messages.g.dart',
  cppHeaderOut: 'webos/messages.g.h',
  cppSourceOut: 'webos/messages.g.cc',
  cppOptions: CppOptions(namespace: 'firebase_core_webos'),
  dartPackageName: 'firebase_core_webos',
))


/// All configurable options for a Firebase App.
class FirebaseOptionsPigeon {
  FirebaseOptionsPigeon({
    required this.apiKey,
    required this.appId,
    required this.projectId,
    this.messagingSenderId,
    this.databaseUrl,
    this.storageBucket,
    this.authDomain,
  });

  String apiKey;
  String appId;
  String projectId;
  String? messagingSenderId;
  String? databaseUrl;
  String? storageBucket;
  String? authDomain;
}

/// Represents an initialized Firebase App returned to Dart.
class FirebaseAppPigeon {
  FirebaseAppPigeon({
    required this.name,
    required this.options,
  });

  /// App name – "[DEFAULT]" for the default app.
  String name;

  /// The options that were used to initialize this app.
  FirebaseOptionsPigeon options;
}

@HostApi()
abstract class FirebaseCoreHostApi {
  @async
  FirebaseAppPigeon initializeApp(
      String appName, FirebaseOptionsPigeon options);

  @async
  FirebaseAppPigeon getApp(String appName);

  @async
  void deleteApp(String appName);
}
