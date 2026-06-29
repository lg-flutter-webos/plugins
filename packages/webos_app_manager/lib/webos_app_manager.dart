// Copyright (c) 2026 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'src/app_manager_method_channel.dart';

class WebOSAppManager {
  final _channel = AppManagerMethodChannel();

  Future<AppLoadStatus?> getAppLoadStatus(String appId) =>
      _channel.getAppLoadStatus(appId);

  static void registerWith() {}
}

class AppLoadStatus {
  const AppLoadStatus({required this.exist});

  static AppLoadStatus? fromMap(Map<dynamic, dynamic> map) {
    try {
      return AppLoadStatus(exist: map['exist'] as bool);
    } catch (_) {
      return null;
    }
  }

  final bool exist;

  @override
  bool operator ==(Object other) =>
      identical(this, other) || other is AppLoadStatus && other.exist == exist;

  @override
  int get hashCode => exist.hashCode;

  @override
  String toString() => 'AppLoadStatus(exist: $exist)';
}
