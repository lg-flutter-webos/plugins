import 'dart:async';
import 'dart:typed_data';
import 'dart:io';
import 'firebase_storage_flutter_api_impl.dart';
import 'storage_task_webos.dart';
import 'task_snapshot_webos.dart';
import 'task_registry.dart';

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_storage_platform_interface/firebase_storage_platform_interface.dart';
import 'package:flutter/services.dart';

import 'messages.g.dart';

class FirebaseStorageWebos extends FirebaseStoragePlatform {

  static bool _callbacksInitialized = false;

  static void ensureCallbacksInitialized() {
    if (_callbacksInitialized) {
      return;
    }

    FirebaseStorageFlutterApi.setup(
      FirebaseStorageFlutterApiImpl(),
    );

    _callbacksInitialized = true;
  }

  FirebaseStorageWebos({
    FirebaseApp? appInstance,
    required String bucket,
  }) : super(appInstance: appInstance, bucket: bucket) {
    ensureCallbacksInitialized();
  }

  static void registerWith() {
    FirebaseStoragePlatform.instance = FirebaseStorageWebos._empty();
  }

  FirebaseStorageWebos._empty() : super(appInstance: null, bucket: '');

  final FirebaseStorageHostApi _hostApi = FirebaseStorageHostApi();

  // Local state for retry time getters
  int _maxOperationRetryTime = 120000; // 2 minutes default
  int _maxUploadRetryTime = 600000; // 10 minutes default
  int _maxDownloadRetryTime = 600000; // 10 minutes default

  @override
  FirebaseStoragePlatform delegateFor(
      {required FirebaseApp app, required String bucket}) {
    return FirebaseStorageWebos(appInstance: app, bucket: bucket);
  }

  @override
  ReferencePlatform getReferenceByPath(String path) {
    return ReferenceWebos(this, path);
  }

  @override
  ReferencePlatform ref([String? path]) {
    return getReferenceByPath(path ?? '/');
  }

  @override
  ReferencePlatform refFromURL(String url) {
    // Handle gs:// URLs
    if (url.startsWith('gs://')) {
      final uri = Uri.parse(url);
      // gs://bucket/path/to/file
      final refPath = uri.path.startsWith('/') ? uri.path.substring(1) : uri.path;
      return ReferenceWebos(this, refPath);
    }

    // Handle https://firebasestorage.googleapis.com URLs
    if (url.contains('firebasestorage.googleapis.com')) {
      final uri = Uri.parse(url);
      // Path format: /v0/b/<bucket>/o/<encoded-path>
      final segments = uri.pathSegments;
      final oIndex = segments.indexOf('o');
      if (oIndex != -1 && oIndex + 1 < segments.length) {
        final encodedPath = segments.sublist(oIndex + 1).join('/');
        final decodedPath = Uri.decodeComponent(encodedPath);
        return ReferenceWebos(this, decodedPath);
      }
    }

    throw ArgumentError('refFromURL: unsupported URL format: $url');
  }

  @override
  int get maxOperationRetryTime => _maxOperationRetryTime;

  @override
  int get maxUploadRetryTime => _maxUploadRetryTime;

  @override
  int get maxDownloadRetryTime => _maxDownloadRetryTime;

  @override
  void setMaxOperationRetryTime(int time) {
    _maxOperationRetryTime = time;
    _hostApi.setMaxOperationRetryTime(app.name, bucket, time);
  }

  @override
  void setMaxUploadRetryTime(int time) {
    _maxUploadRetryTime = time;
    _hostApi.setMaxUploadRetryTime(app.name, bucket, time);
  }

  @override
  void setMaxDownloadRetryTime(int time) {
    _maxDownloadRetryTime = time;
    _hostApi.setMaxDownloadRetryTime(app.name, bucket, time);
  }

  @override
  Future<void> useStorageEmulator(String host, int port) {
    emulatorHost = host;
    emulatorPort = port;
    return _hostApi.useStorageEmulator(app.name, bucket, host, port);
  }
}

class ReferenceWebos extends ReferencePlatform {
  ReferenceWebos(
    FirebaseStoragePlatform storage,
    this.path,
  ) : super(storage, path);
  final String path;

  final FirebaseStorageHostApi _hostApi = FirebaseStorageHostApi();

  @override
  Future<String> getDownloadURL() async {
    return _hostApi.referenceGetDownloadURL(
        storage.app.name, storage.bucket, fullPath);
  }

  @override
  Future<void> delete() async {
    return _hostApi.referenceDelete(storage.app.name, storage.bucket, fullPath);
  }

  @override
  Future<Uint8List?> getData(int maxSize) async {
    return _hostApi.referenceGetData(
        storage.app.name, storage.bucket, fullPath, maxSize);
  }

  @override
  String get bucket => storage.bucket;

  @override
  String get fullPath => path;

  @override
  String get name => path.isEmpty ? '' : path.split('/').last;

  @override
  ReferencePlatform? get parent {
    final normalized = path.endsWith('/')
        ? path.substring(0, path.length - 1)
        : path;
    final parts = normalized.split('/');
    if (parts.length <= 1) return null;
    return ReferenceWebos(storage, parts.sublist(0, parts.length - 1).join('/'));
  }

  @override
  ReferencePlatform get root => ReferenceWebos(storage, '');

  @override
  ReferencePlatform child(String childPath) {
    final newPath = path.isEmpty ? childPath : '$path/$childPath';
    return ReferenceWebos(storage, newPath);
  }

  @override
  Future<ListResultPlatform> list([ListOptions? options]) async {
    return ListResultWebos(storage: storage);
  }

  @override
  Future<ListResultPlatform> listAll() async {
    return ListResultWebos(storage: storage);
  }

  @override
  TaskPlatform putData(Uint8List data, [SettableMetadata? metadata]) {
    final handle = DateTime.now().microsecondsSinceEpoch.toString();
    final task = StorageTaskWebos(handle, _hostApi);
    task.reference = this;
    task.addSnapshot(TaskSnapshotWebos(
      refPlatform: this,
      bytes: 0,
      total: data.length,
      state: TaskState.running,
    ));
    TaskRegistry.register(handle, task);

    _hostApi
        .taskStartPutData(
      storage.app.name,
      storage.bucket,
      fullPath,
      metadata == null ? null : _toPigeonMetadata(metadata),
      data,
      handle,
    )
        .catchError((e) {
      task.addError(e);
      TaskRegistry.remove(handle);
    });

    return task;
  }

  @override
  TaskPlatform putString(String data, PutStringFormat format,
      [SettableMetadata? metadata]) {
    final handle = DateTime.now().microsecondsSinceEpoch.toString();
    final task = StorageTaskWebos(handle, _hostApi);
    task.reference = this;
    task.addSnapshot(TaskSnapshotWebos(
      refPlatform: this,
      bytes: 0,
      total: data.length,
      state: TaskState.running,
    ));
    TaskRegistry.register(handle, task);

    _hostApi
        .taskStartPutString(
      storage.app.name,
      storage.bucket,
      fullPath,
      data,
      PigeonStringFormat.values[format.index],
      metadata == null ? null : _toPigeonMetadata(metadata),
      handle,
    )
        .catchError((e) {
      task.addError(e);
      TaskRegistry.remove(handle);
    });

    return task;
  }

  @override
  TaskPlatform putFile(
    File file, [
    SettableMetadata? metadata,
  ]) {
    final handle = DateTime.now().microsecondsSinceEpoch.toString();

    final task = StorageTaskWebos(handle, _hostApi);
    task.reference = this;

    // Initial snapshot
    task.addSnapshot(TaskSnapshotWebos(
      refPlatform: this,
      bytes: 0,
      total: file.lengthSync(),
      state: TaskState.running,
    ));

    TaskRegistry.register(handle, task);

    _hostApi
        .taskStartPutFile(
      storage.app.name,
      storage.bucket,
      fullPath,
      file.path,
      metadata == null ? null : _toPigeonMetadata(metadata),
      handle,
    )
        .catchError((e) {
      task.addError(e);
      TaskRegistry.remove(handle);
    });

    return task;
  }

  @override
  TaskPlatform writeToFile(File file) {
    final handle = DateTime.now().microsecondsSinceEpoch.toString();
    final task = StorageTaskWebos(handle, _hostApi);
    task.reference = this;
    task.addSnapshot(TaskSnapshotWebos(
      refPlatform: this,
      bytes: 0,
      total: 0,
      state: TaskState.running,
    ));
    TaskRegistry.register(handle, task);

    _hostApi
        .taskStartWriteToFile(
            storage.app.name, storage.bucket, fullPath, file.path, handle)
        .catchError((e) {
      task.addError(e);
      TaskRegistry.remove(handle);
    });

    return task;
  }

  @override
  Future<FullMetadata> getMetadata() async {
    final pigeonMetadata = await _hostApi.referenceGetMetaData(
        storage.app.name, storage.bucket, fullPath);
    return FullMetadata(Map<String, dynamic>.from(pigeonMetadata.metadata!));
  }

  @override
  Future<FullMetadata> updateMetadata(SettableMetadata metadata) async {
    final pigeonMetadata = await _hostApi.referenceUpdateMetaData(
        storage.app.name, storage.bucket, fullPath, _toPigeonMetadata(metadata));
    return FullMetadata(Map<String, dynamic>.from(pigeonMetadata.metadata!));
  }

  PigeonSettableMetaData _toPigeonMetadata(SettableMetadata metadata) {
    return PigeonSettableMetaData(
      contentType: metadata.contentType,
      cacheControl: metadata.cacheControl,
      contentDisposition: metadata.contentDisposition,
      contentEncoding: metadata.contentEncoding,
      contentLanguage: metadata.contentLanguage,
      customMetadata: metadata.customMetadata,
    );
  }
}

class ListResultWebos extends ListResultPlatform {
  ListResultWebos({
    FirebaseStoragePlatform? storage,
    List<ReferencePlatform>? items,
    List<ReferencePlatform>? prefixes,
    String? nextPageToken,
  })  : _items = items ?? [],
        _prefixes = prefixes ?? [],
        super(storage, nextPageToken);

  final List<ReferencePlatform> _items;
  final List<ReferencePlatform> _prefixes;

  @override
  List<ReferencePlatform> get items => _items;

  @override
  List<ReferencePlatform> get prefixes => _prefixes;
}

