import 'package:sqflite/sqflite.dart';

class SqfliteWebOS {
  static void registerWith() {
    databaseFactory = databaseFactorySqflitePlugin;
  }
}
