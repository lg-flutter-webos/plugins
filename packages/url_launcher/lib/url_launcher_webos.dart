// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:core';

import 'package:url_launcher_platform_interface/link.dart';
import 'package:url_launcher_platform_interface/url_launcher_platform_interface.dart';

import 'src/_launcher_client.dart';

class UrlLauncherWebOSPlugin extends UrlLauncherPlatform {
  static final Set<String> _supportedSchemes = <String>{'http', 'https'};

  final LauncherClient _client = LauncherClient();

  UrlLauncherWebOSPlugin();

  static void registerWith() {
    UrlLauncherPlatform.instance = UrlLauncherWebOSPlugin();
  }

  @override
  final LinkDelegate? linkDelegate = null;

  String? _getUrlScheme(String url) => Uri.tryParse(url)?.scheme;

  @override
  Future<bool> canLaunch(String url) async {
    return _supportedSchemes.contains(_getUrlScheme(url));
  }

  @override
  Future<bool> launch(
    String url, {
    required bool useSafariVC,
    required bool useWebView,
    required bool enableJavaScript,
    required bool enableDomStorage,
    required bool universalLinksOnly,
    required Map<String, String> headers,
    String? webOnlyWindowName,
  }) {
    return _client.launch(url);
  }

  @override
  Future<bool> supportsMode(PreferredLaunchMode mode) async {
    return mode == PreferredLaunchMode.platformDefault ||
        mode == PreferredLaunchMode.externalApplication;
  }

  @override
  Future<bool> supportsCloseForMode(PreferredLaunchMode mode) async {
    // No supported mode is closeable.
    return false;
  }
}
