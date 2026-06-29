// Copyright (c) 2023 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:path/path.dart' as path;
import 'package:path_provider_platform_interface/path_provider_platform_interface.dart';

/// The WebOS implementation of [PathProviderPlatform]
///
/// This class implements the `package:path_provider` functionality for WebOS.
class PathProviderWebOS extends PathProviderPlatform {
  /// Constructs an instance of [PathProviderWebOS]
  PathProviderWebOS() : _environment = Platform.environment;

  /// Constructs an instance of [PathProviderWebOS] with the given [environment]
  @visibleForTesting
  PathProviderWebOS.private({
    Map<String, String> environment = const <String, String>{},
  }) : _environment = environment;

  final Map<String, String> _environment;

  /// Registers this class as the default instance of [PathProviderPlatform]
  static void registerWith() {
    PathProviderPlatform.instance = PathProviderWebOS();
  }

  // Get a Directory from Environment variable
  // if not defined, create a directory with allback path
  Directory _getDirectoryFromEnvironment(String envVar, String fallback) {
    ArgumentError.checkNotNull(envVar);
    final String? value = _environment[envVar];
    if (value == null || value.isEmpty) {
      return _getDirectory(fallback);
    }
    return Directory(value);
  }

  // Creates a Directory from a fallback path.
  Directory _getDirectory(String subdir) {
    ArgumentError.checkNotNull(subdir);
    assert(subdir.isNotEmpty);
    final String? homeDir = _environment['FLUTTER_HOME'];
    if (homeDir == null || homeDir.isEmpty) {
      throw StateError(
        'The "FLUTTER_HOME" environment variable is not set. This package '
        'requires that FLUTTER_HOME be set.',
      );
    }
    return Directory(path.joinAll(<String>[homeDir, subdir]));
  }

  Directory get tmpHome =>
      _getDirectoryFromEnvironment('FLUTTER_TEMP_HOME', '.cache');

  Directory get downloadHome =>
      _getDirectoryFromEnvironment('FLUTTER_DOWNLOAD_HOME', '.download');

  Directory get appHome =>
      _getDirectoryFromEnvironment('FLUTTER_APPDATA_HOME', '.data');

  Directory get storagePath =>
      _getDirectoryFromEnvironment('FLUTTER_EXT_STORAGE_PATH', 'data');

  @override
  Future<String?> getTemporaryPath() async {
    final Directory directory = Directory(
      path.join(tmpHome.path, await _getId()),
    );
    await directory.create(recursive: true);
    return directory.path;
  }

  @override
  Future<String?> getApplicationSupportPath() async {
    final Directory directory = Directory(
      path.join(appHome.path, await _getId()),
    );
    await directory.create(recursive: true);
    return directory.path;
  }

  @override
  Future<String?> getApplicationDocumentsPath() async {
    final Directory directory = Directory(
      path.join(appHome.path, await _getId()),
    );
    await directory.create(recursive: true);
    return directory.path;
  }

  @override
  Future<String?> getDownloadsPath() async {
    final Directory directory = Directory(
      path.join(downloadHome.path, await _getId()),
    );

    await directory.create(recursive: true);
    return directory.path;
  }

  @override
  Future<String?> getExternalStoragePath() async {
    return Future<String?>.value(storagePath.path);
  }

  // Gets the unique ID for this application.
  Future<String?> _getId() async {
    final String environmentAppId = _environment['FLUTTER_APP_ID'] ?? '';
    return Future<String?>.value(
      environmentAppId.isEmpty ? 'Default' : environmentAppId,
    );
  }
}
