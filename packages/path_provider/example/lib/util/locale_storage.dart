import 'dart:io';

import 'package:path_provider/path_provider.dart';

class LocaleStorage {
  final String _fileName;

  LocaleStorage(this._fileName);

  Future<String> get _localPath async {
    final Directory directory = await getApplicationDocumentsDirectory();
    return directory.path;
  }

  Future<File> get _localFile async {
    final String path = await _localPath;
    return File('$path/$_fileName');
  }

  Future<String> readFile() async {
    try {
      final File file = await _localFile;

      return await file.readAsString();
    } catch (e) {
      rethrow;
    }
  }

  Future<File> writeFile(String contents) async {
    try {
      final File file = await _localFile;

      return await file.writeAsString(contents);
    } catch (e) {
      rethrow;
    }
  }

  Future<FileSystemEntity> deleteFile() async {
    try {
      final File file = await _localFile;

      return await file.delete();
    } catch (e) {
      rethrow;
    }
  }
}
