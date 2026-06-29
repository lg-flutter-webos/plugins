// Copyright 2024 LGE Corporation. All rights reserved.
// Copyright 2023 Sony Group Corporation. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:convert' show json;

import 'package:flutter/services.dart';
import 'package:shared_preferences_platform_interface/shared_preferences_platform_interface.dart';
import 'package:shared_preferences_platform_interface/types.dart';
import 'shared_preferences_async_webos.dart';

class SharedPreferencesWebos extends SharedPreferencesStorePlatform {
  @Deprecated('Use `SharedPreferencesStorePlatform.instance` instead.')
  static SharedPreferencesWebos instance = SharedPreferencesWebos();

  static void registerWith() {
    SharedPreferencesStorePlatform.instance = SharedPreferencesWebos();

    // forced SharedPreferencesAsync registerWith
    SharedPreferencesAsyncWebos.registerWith();
  }

  final _channel = MethodChannel('webos/shared_preferences');
  static const String _defaultPrefix = 'flutter.';
  Map<String, Object>? _cachedPreferences;

  Future<Map<String, Object>> _readPreferences() async {
    return _cachedPreferences ?? await _reload();
  }

  Future<Map<String, Object>> _reload() async {
    Map<String, Object> preferences = <String, Object>{};

    String? stringMap = await _channel.invokeMethod<String>('load', {});
    if (stringMap != null && stringMap.isEmpty == false) {
      final Object? data = json.decode(stringMap);
      if (data is Map) {
        preferences = data.cast<String, Object>();
      }
    }
    _cachedPreferences = preferences;
    return preferences;
  }

  Future<bool> _writePreferences(Map<String, Object> preferences) async {
    final String stringMap = json.encode(preferences);

    bool? result = await _channel.invokeMethod<bool>('save', {
      'data': stringMap,
    });
    return result ?? false;
  }

  @override
  Future<bool> clear() async {
    return clearWithParameters(
      ClearParameters(filter: PreferencesFilter(prefix: _defaultPrefix)),
    );
  }

  @override
  Future<bool> clearWithPrefix(String prefix) async {
    return clearWithParameters(
      ClearParameters(filter: PreferencesFilter(prefix: prefix)),
    );
  }

  @override
  Future<bool> clearWithParameters(ClearParameters parameters) async {
    final PreferencesFilter filter = parameters.filter;
    final Map<String, Object> preferences = await _readPreferences();
    preferences.removeWhere(
      (String key, _) =>
          key.startsWith(filter.prefix) &&
          (filter.allowList == null || filter.allowList!.contains(key)),
    );
    return _writePreferences(preferences);
  }

  @override
  Future<Map<String, Object>> getAll() async {
    return getAllWithParameters(
      GetAllParameters(filter: PreferencesFilter(prefix: _defaultPrefix)),
    );
  }

  @override
  Future<Map<String, Object>> getAllWithPrefix(String prefix) async {
    return getAllWithParameters(
      GetAllParameters(filter: PreferencesFilter(prefix: prefix)),
    );
  }

  @override
  Future<Map<String, Object>> getAllWithParameters(
    GetAllParameters parameters,
  ) async {
    final PreferencesFilter filter = parameters.filter;
    final Map<String, Object> withPrefix = Map<String, Object>.from(
      await _readPreferences(),
    );
    withPrefix.removeWhere(
      (String key, _) =>
          !(key.startsWith(filter.prefix) &&
              (filter.allowList?.contains(key) ?? true)),
    );
    return withPrefix;
  }

  @override
  Future<bool> remove(String key) async {
    final Map<String, Object> preferences = await _readPreferences();
    preferences.remove(key);
    return _writePreferences(preferences);
  }

  @override
  Future<bool> setValue(String valueType, String key, Object value) async {
    final Map<String, Object> preferences = await _readPreferences();
    preferences[key] = value;
    return _writePreferences(preferences);
  }
}
