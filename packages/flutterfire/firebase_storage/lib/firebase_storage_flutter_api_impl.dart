import 'messages.g.dart';
import 'package:firebase_storage_platform_interface/firebase_storage_platform_interface.dart';
import 'task_registry.dart';
import 'task_snapshot_webos.dart';

class FirebaseStorageFlutterApiImpl
    extends FirebaseStorageFlutterApi {

  @override
  void onTaskEvent(
    String handle,
    PigeonTaskSnapshot snapshot,
  ) {
    final task = TaskRegistry.get(handle);

    if (task == null) {
      return;
    }

    final state = TaskState.values[snapshot.state ?? 1];

    final taskSnapshot = TaskSnapshotWebos(
      refPlatform: task.reference,
      bytes: snapshot.bytesTransferred ?? 0,
      total: snapshot.totalBytes ?? 0,
      state: state,
      metadata: snapshot.metadata?.metadata == null
          ? null
          : FullMetadata(Map<String, dynamic>.from(snapshot.metadata!.metadata!)),
    );

    task.addSnapshot(taskSnapshot);

    if (state == TaskState.success) {
      if (!task.completer.isCompleted) {
        task.completer.complete(taskSnapshot);
      }
      task.close();
      TaskRegistry.remove(handle);
    } else if (state == TaskState.canceled) {
      task.addError(Exception('Task was canceled'));
      task.close();
      TaskRegistry.remove(handle);
    }
  }

  @override
  void onTaskError(
    String handle,
    int errorCode,
    String message,
  ) {
    final task = TaskRegistry.get(handle);

    if (task == null) {
      return;
    }

    task.addError(Exception(message));
    task.close();
    TaskRegistry.remove(handle);
  }
}
