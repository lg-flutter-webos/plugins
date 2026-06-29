// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:io';

import 'package:flutter/material.dart';
import 'package:path_provider/path_provider.dart';
import 'package:path_provider_example/util/locale_storage.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Path Provider',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: const MyHomePage(title: 'Path Provider'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});
  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  Future<Directory?>? _tempDirectory;
  Future<Directory?>? _appSupportDirectory;
  Future<Directory?>? _appLibraryDirectory;
  Future<Directory?>? _appDocumentsDirectory;
  Future<Directory?>? _externalDocumentsDirectory;
  Future<List<Directory>?>? _externalStorageDirectories;
  Future<List<Directory>?>? _externalCacheDirectories;
  Future<Directory?>? _downloadsDirectory;
  Future<String?>? _fileContent;

  final LocaleStorage localeStorage = LocaleStorage('file_content');

  void _requestTempDirectory() {
    setState(() {
      _tempDirectory = getTemporaryDirectory();
    });
  }

  Widget _buildString(BuildContext context, AsyncSnapshot<String?> snapshot) {
    Text text = const Text('');
    if (snapshot.connectionState == ConnectionState.done) {
      if (snapshot.hasError) {
        text = Text('Error: ${snapshot.error}');
      } else if (snapshot.hasData) {
        text = Text('Result : ${snapshot.data}');
      } else {
        text = const Text('unavailable');
      }
    }
    return Padding(padding: const EdgeInsets.all(16.0), child: text);
  }

  Widget _buildDirectory(
      BuildContext context, AsyncSnapshot<Directory?> snapshot) {
    Text text = const Text('');
    if (snapshot.connectionState == ConnectionState.done) {
      if (snapshot.hasError) {
        text = Text('Error: ${snapshot.error}');
      } else if (snapshot.hasData) {
        text = Text('path: ${snapshot.data!.path}');
      } else {
        text = const Text('path unavailable');
      }
    }
    return Padding(padding: const EdgeInsets.all(16.0), child: text);
  }

  Widget _buildDirectories(
      BuildContext context, AsyncSnapshot<List<Directory>?> snapshot) {
    Text text = const Text('');
    if (snapshot.connectionState == ConnectionState.done) {
      if (snapshot.hasError) {
        text = Text('Error: ${snapshot.error}');
      } else if (snapshot.hasData) {
        final String combined =
            snapshot.data!.map((Directory d) => d.path).join(', ');
        text = Text('paths: $combined');
      } else {
        text = const Text('path unavailable');
      }
    }
    return Padding(padding: const EdgeInsets.all(16.0), child: text);
  }

  void _requestAppDocumentsDirectory() {
    setState(() {
      _appDocumentsDirectory = getApplicationDocumentsDirectory();
    });
  }

  void _requestAppSupportDirectory() {
    setState(() {
      _appSupportDirectory = getApplicationSupportDirectory();
    });
  }

  void _requestAppLibraryDirectory() {
    setState(() {
      _appLibraryDirectory = getLibraryDirectory();
    });
  }

  void _requestExternalStorageDirectory() {
    setState(() {
      _externalDocumentsDirectory = getExternalStorageDirectory();
    });
  }

  void _requestExternalStorageDirectories(StorageDirectory type) {
    setState(() {
      _externalStorageDirectories = getExternalStorageDirectories(type: type);
    });
  }

  void _requestExternalCacheDirectories() {
    setState(() {
      _externalCacheDirectories = getExternalCacheDirectories();
    });
  }

  void _requestDownloadsDirectory() {
    setState(() {
      _downloadsDirectory = getDownloadsDirectory();
    });
  }

  void _writeFile() async {
    try {
      File result = await localeStorage.writeFile('Writing a file is done');
      setState(() {
        _fileContent = Future<String>.value(
            'Writing a file is done : ${result.toString()}');
      });
    } catch (e) {
      setState(() {
        _fileContent = Future<String>.value('Writing a file is failed : $e');
      });
    }
  }

  void _readFile() async {
    try {
      String result = await localeStorage.readFile();
      setState(() {
        _fileContent = Future<String>.value('Reading a file is done : $result');
      });
    } catch (e) {
      setState(() {
        _fileContent = Future<String>.value('Reading a file is failed : $e');
      });
    }
  }

  void _deleteFile() async {
    try {
      FileSystemEntity result = await localeStorage.deleteFile();
      setState(() {
        _fileContent = Future<String>.value(
            'Deleting a file is done : ${result.toString()}');
      });
    } catch (e) {
      setState(() {
        _fileContent = Future<String>.value('Deleting a file is failed : $e');
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: Center(
        child: ListView(
          children: <Widget>[
            Column(
              children: <Widget>[
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: ElevatedButton(
                    onPressed: _requestTempDirectory,
                    child: const Text(
                      'Get Temporary Directory',
                    ),
                  ),
                ),
                FutureBuilder<Directory?>(
                  future: _tempDirectory,
                  builder: _buildDirectory,
                ),
              ],
            ),
            Column(
              children: <Widget>[
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: ElevatedButton(
                    onPressed: _requestAppDocumentsDirectory,
                    child: const Text(
                      'Get Application Documents Directory',
                    ),
                  ),
                ),
                FutureBuilder<Directory?>(
                  future: _appDocumentsDirectory,
                  builder: _buildDirectory,
                ),
              ],
            ),
            Column(
              children: <Widget>[
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: ElevatedButton(
                    onPressed: _requestAppSupportDirectory,
                    child: const Text(
                      'Get Application Support Directory',
                    ),
                  ),
                ),
                FutureBuilder<Directory?>(
                  future: _appSupportDirectory,
                  builder: _buildDirectory,
                ),
              ],
            ),
            Column(
              children: <Widget>[
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: ElevatedButton(
                    onPressed:
                        Platform.isAndroid ? null : _requestAppLibraryDirectory,
                    child: Text(
                      Platform.isAndroid
                          ? 'Application Library Directory unavailable'
                          : 'Get Application Library Directory',
                    ),
                  ),
                ),
                FutureBuilder<Directory?>(
                  future: _appLibraryDirectory,
                  builder: _buildDirectory,
                ),
              ],
            ),
            Column(
              children: <Widget>[
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: ElevatedButton(
                    onPressed: _requestExternalStorageDirectory,
                    child: const Text(
                      'Get External Storage Directory',
                    ),
                  ),
                ),
                FutureBuilder<Directory?>(
                  future: _externalDocumentsDirectory,
                  builder: _buildDirectory,
                ),
              ],
            ),
            Column(
              children: <Widget>[
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: ElevatedButton(
                    onPressed: !Platform.isAndroid
                        ? null
                        : () {
                            _requestExternalStorageDirectories(
                              StorageDirectory.music,
                            );
                          },
                    child: Text(
                      !Platform.isAndroid
                          ? 'External directories are unavailable'
                          : 'Get External Storage Directories',
                    ),
                  ),
                ),
                FutureBuilder<List<Directory>?>(
                  future: _externalStorageDirectories,
                  builder: _buildDirectories,
                ),
              ],
            ),
            Column(
              children: <Widget>[
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: ElevatedButton(
                    onPressed: !Platform.isAndroid
                        ? null
                        : _requestExternalCacheDirectories,
                    child: Text(
                      !Platform.isAndroid
                          ? 'External directories are unavailable'
                          : 'Get External Cache Directories',
                    ),
                  ),
                ),
                FutureBuilder<List<Directory>?>(
                  future: _externalCacheDirectories,
                  builder: _buildDirectories,
                ),
              ],
            ),
            Column(
              children: <Widget>[
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: ElevatedButton(
                    onPressed: Platform.isAndroid || Platform.isIOS
                        ? null
                        : _requestDownloadsDirectory,
                    child: Text(
                      Platform.isAndroid || Platform.isIOS
                          ? 'Downloads directory is unavailable'
                          : 'Get Downloads Directory',
                    ),
                  ),
                ),
                FutureBuilder<Directory?>(
                  future: _downloadsDirectory,
                  builder: _buildDirectory,
                ),
              ],
            ),
            Column(
              children: <Widget>[
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: <Widget>[
                      ElevatedButton(
                        onPressed: _writeFile,
                        child: const Text('Write'),
                      ),
                      const SizedBox(
                        width: 10,
                      ),
                      ElevatedButton(
                        onPressed: _readFile,
                        child: const Text('Read'),
                      ),
                      const SizedBox(
                        width: 10,
                      ),
                      ElevatedButton(
                        onPressed: _deleteFile,
                        child: const Text('Delete'),
                      ),
                    ],
                  ),
                ),
                FutureBuilder<String?>(
                  future: _fileContent,
                  builder: _buildString,
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
