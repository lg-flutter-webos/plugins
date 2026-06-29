// ignore_for_file: require_trailing_commas

import 'package:firebase_core_platform_interface/firebase_core_platform_interface.dart';
import 'src/messages.g.dart';

class FirebaseAppWebos extends FirebaseAppPlatform {
  final String _name;
  final FirebaseOptions _options;
  final FirebaseCoreHostApi _api;
  final Map<String, FirebaseAppPlatform> _cache;

  FirebaseAppWebos(
    this._name,
    this._options,
    this._api,
    this._cache,
  ) : super(_name, _options);

  @override
  String get name => _name;

  @override
  FirebaseOptions get options => _options;

  @override
  Future<void> delete() async {
    await _api.deleteApp(_name);
    _cache.remove(_name);
  }
}