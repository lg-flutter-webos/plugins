// SPDX-FileCopyrightText: Copyright 2024 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause
import 'dart:async';

import 'package:connectivity_plus_platform_interface/connectivity_plus_platform_interface.dart';

import 'src/_connectivity_client.dart';

class ConnectivityPlusWebos extends ConnectivityPlatform {
  ConnectivityPlusWebos();

  final ConnectivityClient _client = ConnectivityClient();

  static void registerWith() {
    ConnectivityPlatform.instance = ConnectivityPlusWebos();
  }

  @override
  Future<List<ConnectivityResult>> checkConnectivity() => _client.current();

  @override
  Stream<List<ConnectivityResult>> get onConnectivityChanged =>
      _client.statusStream;
}
