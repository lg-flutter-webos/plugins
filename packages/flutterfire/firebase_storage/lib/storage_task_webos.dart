import 'dart:async';
import 'package:firebase_storage_platform_interface/firebase_storage_platform_interface.dart';
import 'messages.g.dart';

class StorageTaskWebos extends TaskPlatform {
  StorageTaskWebos(this._handle, this._hostApi) {
    _streamController = StreamController<TaskSnapshotPlatform>.broadcast();
  }

  final String _handle;
  final FirebaseStorageHostApi _hostApi;

  late final StreamController<TaskSnapshotPlatform> _streamController;
  late TaskSnapshotPlatform _lastSnapshot;
  late ReferencePlatform reference;

  final Completer<TaskSnapshotPlatform> completer =
      Completer<TaskSnapshotPlatform>();

  void addSnapshot(TaskSnapshotPlatform snapshot) {
    _lastSnapshot = snapshot;
    _streamController.add(snapshot);
  }

  void addError(Object error, [StackTrace? stackTrace]) {
    _streamController.addError(error, stackTrace);
    if (!completer.isCompleted) {
      completer.completeError(error, stackTrace);
    }
  }

  void close() {
    _streamController.close();
  }

  @override
  Future<TaskSnapshotPlatform> get onComplete => completer.future;

  @override
  Stream<TaskSnapshotPlatform> get snapshotEvents => _streamController.stream;

  @override
  TaskSnapshotPlatform get snapshot => _lastSnapshot;

  @override
  Future<bool> pause() async {
    try {
      await _hostApi.taskPause(_handle);
      return true;
    } catch (e) {
      return false;
    }
  }

  @override
  Future<bool> resume() async {
    try {
      await _hostApi.taskResume(_handle);
      return true;
    } catch (e) {
      return false;
    }
  }

  @override
  Future<bool> cancel() async {
    try {
      await _hostApi.taskCancel(_handle);
      return true;
    } catch (e) {
      return false;
    }
  }
}
