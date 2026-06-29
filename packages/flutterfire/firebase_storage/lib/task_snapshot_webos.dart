import 'package:firebase_storage_platform_interface/firebase_storage_platform_interface.dart';

class TaskSnapshotWebos extends TaskSnapshotPlatform {
  TaskSnapshotWebos({
    required this.refPlatform,
    required this.bytes,
    required this.total,
    required TaskState state,
    FullMetadata? metadata,
  })  : _metadata = metadata,
        super(state, {});

  final ReferencePlatform refPlatform;
  final int bytes;
  final int total;
  final FullMetadata? _metadata;

  @override
  int get bytesTransferred => bytes;

  @override
  FullMetadata? get metadata => _metadata;

  @override
  ReferencePlatform get ref => refPlatform;

  @override
  int get totalBytes => total;
}
