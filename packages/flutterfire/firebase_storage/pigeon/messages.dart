import 'package:pigeon/pigeon.dart';

@ConfigurePigeon(PigeonOptions(
  dartOut: 'lib/messages.g.dart',
  cppHeaderOut: 'webos/messages.g.h',
  cppSourceOut: 'webos/messages.g.cc',
  cppOptions: CppOptions(namespace: 'firebase_storage_webos'),
  dartPackageName: 'firebase_storage_webos',
))

// ---------------------------------------------------------------------------
// Data classes
// ---------------------------------------------------------------------------

/// Settable metadata fields for an upload.
class PigeonSettableMetaData {
  String? cacheControl;
  String? contentDisposition;
  String? contentEncoding;
  String? contentLanguage;
  String? contentType;
  Map<String?, String?>? customMetadata;
}

/// Full metadata returned by the server.
class PigeonFullMetaData {
  Map<String?, Object?>? metadata;
}

/// Options for listing items under a storage reference.
class PigeonListOptions {
  int? maxResults;
  String? pageToken;
}

/// Result of a list/listAll operation.
class PigeonListResult {
  List<String?>? items;
  List<String?>? prefixs;
  String? pageToken;
}

/// A snapshot of an ongoing transfer task.
class PigeonTaskSnapshot {
  String? handle;
  int? bytesTransferred;
  int? totalBytes;
  /// 0 = paused, 1 = running, 2 = success, 3 = canceled, 4 = error
  int? state;
  PigeonFullMetaData? metadata;
}

// ---------------------------------------------------------------------------
// String format for putString uploads.
// ---------------------------------------------------------------------------

enum PigeonStringFormat {
  raw,
  base64,
  base64Url,
  dataUrl,
}

// ---------------------------------------------------------------------------
// Flutter → C++ (host) API
// ---------------------------------------------------------------------------

@HostApi()
abstract class FirebaseStorageHostApi {
  
  @async
  void setMaxOperationRetryTime(String appName, String bucket, int time);
  
  @async
  void setMaxUploadRetryTime(String appName, String bucket, int time);
  
  @async
  void setMaxDownloadRetryTime(String appName, String bucket, int time);
  
  @async
  void useStorageEmulator(String appName, String bucket, String host, int port);

  @async
  String referenceGetDownloadURL(String appName, String bucket, String path);

  @async
  PigeonFullMetaData referenceGetMetaData(
      String appName, String bucket, String path);

  @async
  PigeonFullMetaData referenceUpdateMetaData(String appName, String bucket,
      String path, PigeonSettableMetaData metadata);

  @async
  void referenceDelete(String appName, String bucket, String path);

  @async
  Uint8List referenceGetData(
      String appName, String bucket, String path, int maxSize);

  @async
  PigeonListResult referenceList(
      String appName, String bucket, String path, PigeonListOptions options);

  @async
  PigeonListResult referenceListAll(String appName, String bucket, String path);

  @async
  void taskStartPutData(String appName, String bucket, String path,
      PigeonSettableMetaData? metadata, Uint8List data, String handle);

  @async
  void taskStartPutString(String appName, String bucket, String path,
      String data, PigeonStringFormat format,
      PigeonSettableMetaData? metadata, String handle);

  @async
  void taskStartPutFile(String appName, String bucket, String path,
      String filePath, PigeonSettableMetaData? metadata, String handle);

  @async
  void taskStartWriteToFile(String appName, String bucket, String path,
      String filePath, String handle);

  @async
  void taskPause(String handle);
  @async
  void taskResume(String handle);
  @async
  void taskCancel(String handle);
}

@FlutterApi()
abstract class FirebaseStorageFlutterApi {
  void onTaskEvent(String handle, PigeonTaskSnapshot snapshot);
  void onTaskError(String handle, int errorCode, String message);
}
