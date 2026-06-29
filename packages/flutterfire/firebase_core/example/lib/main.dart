// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'firebase_options.dart';

void main() => runApp(const MyApp());

class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);

  String get name => 'foo';

  Future<void> initializeDefault() async {
    FirebaseApp app = await Firebase.initializeApp(
      options: DefaultFirebaseOptions.currentPlatform,
    );
  }

  Future<void> initializeDefaultFromWebOS() async {
    if (defaultTargetPlatform == TargetPlatform.linux) {
      FirebaseApp app = await Firebase.initializeApp(
        options: DefaultFirebaseOptions.currentPlatform,
      );
    }
  }

  Future<void> initializeSecondary() async {
    FirebaseApp app = await Firebase.initializeApp(
      name: name,
      options: DefaultFirebaseOptions.currentPlatform,
    );
  }

  void apps() {
    final List<FirebaseApp> apps = Firebase.apps;
  }

  void options() {
    final FirebaseApp app = Firebase.app();
    final options = app.options;
  }

  Future<void> delete() async {
    final FirebaseApp app = Firebase.app(name);
    await app.delete();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Firebase Core example app'),
        ),
        body: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: <Widget>[
              ElevatedButton(
                onPressed: initializeDefault,
                child: const Text('Initialize default app'),
              ),
              if (defaultTargetPlatform == TargetPlatform.linux)
                ElevatedButton(
                  onPressed: initializeDefaultFromWebOS,
                  child: const Text(
                    'Initialize default app from WebOS/Linux resources',
                  ),
                ),
              ElevatedButton(
                onPressed: initializeSecondary,
                child: const Text('Initialize secondary app'),
              ),
              ElevatedButton(
                onPressed: apps,
                child: const Text('List apps'),
              ),
              ElevatedButton(
                onPressed: options,
                child: const Text('List default options'),
              ),
              ElevatedButton(
                onPressed: delete,
                child: const Text('Delete secondary app'),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
