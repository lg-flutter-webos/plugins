// Copyright (c) 2026 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:core';
import 'dart:ui';
import 'package:flutter/widgets.dart';
import 'package:device_info_plus_platform_interface/device_info_plus_platform_interface.dart';

import 'src/_device_info_client.dart';

int _screenWidth = 0;
int _screenHeight = 0;

class DeviceInfoWebOSPlugin extends DeviceInfoPlatform {
  static void registerWith() {
    DeviceInfoPlatform.instance = DeviceInfoWebOSPlugin();
  }

  static WebOSDeviceInfo? _cache;

  Future<WebOSDeviceInfo> get webosInfo async {
    if (_screenWidth == 0 && _screenHeight == 0) {
      FlutterView view = WidgetsBinding.instance.platformDispatcher.views.first;
      _screenWidth = view.display.size.width.round();
      _screenHeight = view.display.size.height.round();
    }
    _cache ??= WebOSDeviceInfo(await fetchDeviceInfo());
    return _cache!;
  }

  @override
  Future<BaseDeviceInfo> deviceInfo() async {
    return await webosInfo;
  }
}

class WebOSDeviceInfo implements BaseDeviceInfo {
  WebOSDeviceInfo(Map<String, dynamic> map) {
    modelName = map['modelName'];
    version = map['version'];
    versionMajor = map['versionMajor'];
    versionMinor = map['versionMinor'];
    versionDot = map['versionDot'];
    sdkVersion = map['sdkVersion'];
    screenWidth = _screenWidth;
    screenHeight = _screenHeight;
    uhd = map['uhd'];
    oled = map['oled'];
    ddrSize = map['ddrSize'];
    hdr10 = map['hdr10'];
    dolbyVision = map['dolbyVision'];
    dolbyAtmos = map['dolbyAtmos'];
    brandName = map['brandName'];
    manufacturer = map['manufacturer'];
  }

  String? modelName;
  String? version;
  int? versionMajor;
  int? versionMinor;
  int? versionDot;
  String? sdkVersion;
  int? screenWidth;
  int? screenHeight;
  bool? uhd;
  bool? oled;
  String? ddrSize;
  bool? hdr10;
  bool? dolbyVision;
  bool? dolbyAtmos;
  String? brandName;
  String? manufacturer;

  @override
  Map<String, dynamic> get data => toMap();

  @override
  Map<String, dynamic> toMap() {
    return {
      'modelName': modelName,
      'version': version,
      'versionMajor': versionMajor,
      'versionMinor': versionMinor,
      'versionDot': versionDot,
      'sdkVersion': sdkVersion,
      'screenWidth': screenWidth,
      'screenHeight': screenHeight,
      'uhd': uhd,
      'oled': oled,
      'ddrSize': ddrSize,
      'hdr10': hdr10,
      'dolbyVision': dolbyVision,
      'dolbyAtmos': dolbyAtmos,
      'brandName': brandName,
      'manufacturer': manufacturer,
    };
  }

  @override
  String toString() {
    return '''
modelName: $modelName
version: $version
versionMajor: $versionMajor
versionMinor: $versionMinor
versionDot: $versionDot
sdkVersion: $sdkVersion
screenWidth: $screenWidth
screenHeight: $screenHeight
uhd: $uhd
oled: $oled
ddrSize: $ddrSize
hdr10: $hdr10
dolbyVision: $dolbyVision
dolbyAtmos: $dolbyAtmos
brandName: $brandName
manufacturer: $manufacturer
''';
  }
}
