// Copyright 2024 LGE Corporation. All rights reserved.
// Copyright 2024 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:convert' show json;

import 'package:flutter/services.dart';
import 'package:shared_preferences_platform_interface/shared_preferences_async_platform_interface.dart';
import 'package:shared_preferences_platform_interface/types.dart';

base class SharedPreferencesAsyncWebos extends SharedPreferencesAsyncPlatform {
  static void registerWith() {
    SharedPreferencesAsyncPlatform.instance = SharedPreferencesAsyncWebos();
  }

  final _channel = MethodChannel('webos/shared_preferences');

  Future<Map<String, Object>> _readPreferences() async {
    Map<String, Object> preferences = <String, Object>{};

    String? stringMap = await _channel.invokeMethod<String>('load', {});
    if (stringMap != null && stringMap.isEmpty == false) {
      final Object? data = json.decode(stringMap);
      if (data is Map) {
        preferences = data.cast<String, Object>();
      }
    }
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
  Future<void> setString(
    String key,
    String value,
    SharedPreferencesOptions options,
  ) async {
    final Map<String, Object> preferences = await _readPreferences();
    preferences[key] = value;
    _writePreferences(preferences);
  }

  @override
  Future<void> setInt(
    String key,
    int value,
    SharedPreferencesOptions options,
  ) async {
    final Map<String, Object> preferences = await _readPreferences();
    preferences[key] = value;
    _writePreferences(preferences);
  }

  @override
  Future<void> setStringList(
    String key,
    List<String> value,
    SharedPreferencesOptions options,
  ) async {
    final Map<String, Object> preferences = await _readPreferences();
    preferences[key] = value;
    _writePreferences(preferences);
  }

  @override
  Future<void> setBool(
    String key,
    bool value,
    SharedPreferencesOptions options,
  ) async {
    final Map<String, Object> preferences = await _readPreferences();
    preferences[key] = value;
    _writePreferences(preferences);
  }

  @override
  Future<void> setDouble(
    String key,
    double value,
    SharedPreferencesOptions options,
  ) async {
    final Map<String, Object> preferences = await _readPreferences();
    preferences[key] = value;
    _writePreferences(preferences);
  }

  @override
  Future<String?> getString(
    String key,
    SharedPreferencesOptions options,
  ) async {
    final Map<String, Object> preferences = await _readPreferences();
    return preferences[key] as String?;
  }

  @override
  Future<bool?> getBool(String key, SharedPreferencesOptions options) async {
    final Map<String, Object> preferences = await _readPreferences();
    return preferences[key] as bool?;
  }

  @override
  Future<double?> getDouble(
    String key,
    SharedPreferencesOptions options,
  ) async {
    final Map<String, Object> preferences = await _readPreferences();
    return preferences[key] as double?;
  }

  @override
  Future<int?> getInt(String key, SharedPreferencesOptions options) async {
    final Map<String, Object> preferences = await _readPreferences();
    return preferences[key] as int?;
  }

  @override
  Future<List<String>?> getStringList(
    String key,
    SharedPreferencesOptions options,
  ) async {
    final Map<String, Object> preferences = await _readPreferences();
    return preferences[key] as List<String>?;
  }

  @override
  Future<void> clear(
    ClearPreferencesParameters parameters,
    SharedPreferencesOptions options,
  ) async {
    final PreferencesFilters filter = parameters.filter;
    final Map<String, Object> preferences = await _readPreferences();
    preferences.removeWhere(
      (String key, _) =>
          (filter.allowList == null || filter.allowList!.contains(key)),
    );
    _writePreferences(preferences);
  }

  @override
  Future<Set<String>> getKeys(
    GetPreferencesParameters parameters,
    SharedPreferencesOptions options,
  ) async {
    Set<String> value = {};
    final PreferencesFilters filter = parameters.filter;
    final Map<String, Object> preferences = await _readPreferences();
    for (var key in preferences.keys) {
      if (filter.allowList == null || filter.allowList!.contains(key)) {
        value.add(key);
      }
    }
    return value;
  }

  @override
  Future<Map<String, Object>> getPreferences(
    GetPreferencesParameters parameters,
    SharedPreferencesOptions options,
  ) async {
    Map<String, Object> value = {};
    final PreferencesFilters filter = parameters.filter;
    final Map<String, Object> preferences = await _readPreferences();

    for (String key in preferences.keys) {
      if (filter.allowList == null || filter.allowList!.contains(key)) {
        value[key] = preferences[key]!;
      }
    }
    return value;
  }
}
