import 'storage_task_webos.dart';

class TaskRegistry {
  static final Map<String, StorageTaskWebos> _tasks = {};

  static void register(
    String handle,
    StorageTaskWebos task,
  ) {
    _tasks[handle] = task;
  }

  static StorageTaskWebos? get(String handle) {
    return _tasks[handle];
  }

  static void remove(String handle) {
    _tasks.remove(handle);
  }
}
