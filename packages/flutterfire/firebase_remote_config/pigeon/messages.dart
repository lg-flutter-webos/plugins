import 'package:pigeon/pigeon.dart';

@ConfigurePigeon(
  PigeonOptions(
    dartOut: 'lib/messages.g.dart',
    cppHeaderOut: 'webos/messages.g.h',
    cppSourceOut: 'webos/messages.g.cc',
    cppOptions: CppOptions(namespace: 'firebase_remote_config_webos'),
    dartPackageName: 'firebase_remote_config_webos',
  ),
)

// ---------------------------------------------------------------------------
// Data classes
// ---------------------------------------------------------------------------

enum PigeonValueSource {
  staticValue,
  defaultValue,
  remoteValue,
}

class PigeonRemoteConfigSettings {
  PigeonRemoteConfigSettings(
    this.fetchTimeout,
    this.minimumFetchInterval,
  );

  double fetchTimeout;
  double minimumFetchInterval;
}

class PigeonRemoteConfigValue {
  PigeonRemoteConfigValue({
    this.value,
    required this.source,
  });

  String? value;
  PigeonValueSource source;
}

// ---------------------------------------------------------------------------
// Host API — Dart → C++
// ---------------------------------------------------------------------------

@HostApi()
abstract class FirebaseRemoteConfigHostApi {
  @async
  void initialize(String appName);

  @async
  void ensureInitialized(String appName);

  @async
  void fetch(
    String appName,
    double cacheExpirationInSeconds,
  );

  @async
  bool activate(String appName);

  @async
  bool fetchAndActivate(String appName);

  PigeonRemoteConfigValue getValue(
    String appName,
    String key,
  );

  String getString(
    String appName,
    String key,
  );

  bool getBool(
    String appName,
    String key,
  );

  int getInt(
    String appName,
    String key,
  );

  double getDouble(
    String appName,
    String key,
  );

  Map<String?, Object?> getAll(String appName);

  @async
  void setDefaults(
    String appName,
    Map<String?, Object?> defaultParameters,
  );

  @async
  void setInitialValues(
    String appName,
    Map<String?, Object?> remoteConfigValues,
  );

  @async
  void setConfigSettings(
    String appName,
    PigeonRemoteConfigSettings settings,
  );

  PigeonRemoteConfigSettings getSettings(String appName);

  PigeonRemoteConfigSettings getConfigSettings(String appName);

  @async
  void setCustomSignals(
    String appName,
    Map<String?, Object?> customSignals,
  );

  int getLastFetchStatus(String appName);

  int getLastFetchTime(String appName);
}