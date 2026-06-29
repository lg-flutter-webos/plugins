import 'dart:async';

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_remote_config_platform_interface/firebase_remote_config_platform_interface.dart';

import 'messages.g.dart';

class FirebaseRemoteConfigWebos extends FirebaseRemoteConfigPlatform {
  final FirebaseApp? _app;
  final FirebaseRemoteConfigHostApi _hostApi =
      FirebaseRemoteConfigHostApi();

  FirebaseApp get app => _app ?? Firebase.app();

  static bool _callbacksInitialized = false;

  static void ensureCallbacksInitialized() {
    if (_callbacksInitialized) {
      return;
    }

    _callbacksInitialized = true;

  }

  // ---------------------------
  // NORMAL CONSTRUCTOR
  // ---------------------------
  FirebaseRemoteConfigWebos({FirebaseApp? app})
      : _app = app,
        super(appInstance: app) {
    ensureCallbacksInitialized();
  }

  // ---------------------------
  // REGISTER
  // ---------------------------
  static void registerWith() {

    FirebaseRemoteConfigPlatform.instance =
        FirebaseRemoteConfigWebos._empty();
  }

  // ---------------------------
  // EMPTY CONSTRUCTOR (IMPORTANT)
  // ---------------------------
  FirebaseRemoteConfigWebos._empty()
      : _app = null,
        super(appInstance: null);

  final Map<String, RemoteConfigValue> _values = {};

  RemoteConfigSettings _settings = RemoteConfigSettings(
    fetchTimeout: Duration.zero,
    minimumFetchInterval: Duration.zero,
  );

  DateTime _lastFetchTime =
      DateTime.fromMillisecondsSinceEpoch(0, isUtc: true);

  RemoteConfigFetchStatus _lastFetchStatus =
      RemoteConfigFetchStatus.noFetchYet;

  @override
  FirebaseRemoteConfigPlatform delegateFor({required FirebaseApp app}) {
    return FirebaseRemoteConfigWebos(app: app);
  }

  @override
  FirebaseRemoteConfigPlatform setInitialValues({
    required Map<dynamic, dynamic> remoteConfigValues,
  }) {
    _values.clear();

    remoteConfigValues.forEach((key, value) {
      if (key is String && value is PigeonRemoteConfigValue) {
        _values[key] = _fromPigeonValue(value);
      }
    });

    return this;
  }

Future<void> _refreshCache() async {
  final rawValues = await _hostApi.getAll(app.name);

  _values.clear();

  rawValues.forEach((key, value) {
    if (key != null && value is PigeonRemoteConfigValue) {
      _values[key!] = _fromPigeonValue(value);
    }
  });

  final settings = await _hostApi.getSettings(app.name);
  _settings = RemoteConfigSettings(
    fetchTimeout: Duration(
      milliseconds: (settings.fetchTimeout * 1000).toInt(),
    ),
    minimumFetchInterval: Duration(
      milliseconds: (settings.minimumFetchInterval * 1000).toInt(),
    ),
  );

  final fetchTime = await _hostApi.getLastFetchTime(app.name);
  _lastFetchTime =
      DateTime.fromMillisecondsSinceEpoch(fetchTime, isUtc: true);

  final fetchStatus = await _hostApi.getLastFetchStatus(app.name);
  _lastFetchStatus = _toFetchStatus(fetchStatus);
  
}

RemoteConfigValue _fromPigeonValue(PigeonRemoteConfigValue value) {
  final result = RemoteConfigValue(
    (value.value ?? '').codeUnits,
    _toValueSource(value.source),
  );
  return result;
}

ValueSource _toValueSource(PigeonValueSource source) {
  switch (source) {
    case PigeonValueSource.staticValue:
      return ValueSource.valueStatic;
    case PigeonValueSource.defaultValue:
      return ValueSource.valueDefault;
    case PigeonValueSource.remoteValue:
      return ValueSource.valueRemote;
  }
}
  RemoteConfigFetchStatus _toFetchStatus(int status) {
    switch (status) {
      case 1:
        return RemoteConfigFetchStatus.success;
      case 2:
        return RemoteConfigFetchStatus.failure;
      case 3:
        return RemoteConfigFetchStatus.throttle;
      default:
        return RemoteConfigFetchStatus.noFetchYet;
    }
  }

  @override
  Future<void> ensureInitialized() async {
    await _hostApi.ensureInitialized(app.name);
    await _refreshCache();
  }

  @override
  Future<bool> activate() async {
    final result = await _hostApi.activate(app.name);
    await _refreshCache();
    return result;
  }

  @override
  Future<void> fetch() async {
    await _hostApi.fetch(app.name, 0);
    await _refreshCache();
  }

  @override
  Future<bool> fetchAndActivate() async {
    final result = await _hostApi.fetchAndActivate(app.name);
    await _refreshCache();
    return result;
  }

  @override
  Map<String, RemoteConfigValue> getAll() {
    return Map<String, RemoteConfigValue>.from(_values);
  }

  @override
  bool getBool(String key) {
    final value = getValue(key);
    return value.asBool();
  }

  @override
  int getInt(String key) {
    final value = getValue(key);
    return value.asInt();
  }

  @override
  double getDouble(String key) {
    final value = getValue(key);
    return value.asDouble();
  }

  @override
  String getString(String key) {
    final value = getValue(key);
    return value.asString();
  }

@override
RemoteConfigValue getValue(String key) {
  final value = _values[key] ??
      RemoteConfigValue(
        <int>[],
        ValueSource.valueStatic,
      );
  return value;
}

  @override
  Future<void> setDefaults(Map<String, dynamic> defaultParameters) async {
    await _hostApi.setDefaults(app.name, defaultParameters);
    await _refreshCache();
  }

@override
Future<void> setConfigSettings(
  RemoteConfigSettings remoteConfigSettings,
) async {
  await _hostApi.setConfigSettings(
    app.name,
    PigeonRemoteConfigSettings(
      fetchTimeout:
          remoteConfigSettings.fetchTimeout.inSeconds.toDouble(),
      minimumFetchInterval:
          remoteConfigSettings.minimumFetchInterval.inSeconds.toDouble(),
    ),
  );

  await _refreshCache();
}

  @override
  Future<void> setCustomSignals(
    Map<String, Object?> customSignals,
  ) {
    return _hostApi.setCustomSignals(app.name, customSignals);
  }

  @override
  DateTime get lastFetchTime => _lastFetchTime;

  @override
  RemoteConfigFetchStatus get lastFetchStatus => _lastFetchStatus;

  @override
  RemoteConfigSettings get settings => _settings;

  @override
  Stream<RemoteConfigUpdate> get onConfigUpdated {
    return const Stream.empty();
  }
}