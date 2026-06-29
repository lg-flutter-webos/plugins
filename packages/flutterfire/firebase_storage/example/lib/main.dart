import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_storage/firebase_storage.dart';
import 'firebase_options.dart';


void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  
  // Initialize Firebase with your WebOS confi
    await Firebase.initializeApp(
      options: DefaultFirebaseOptions.currentPlatform,
    );


  runApp(const WebOSStorageTestApp());
}

class WebOSStorageTestApp extends StatelessWidget {
  const WebOSStorageTestApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Firebase Storage WebOS',
      theme: ThemeData(
        brightness: Brightness.dark,
        primaryColor: const Color(0xFFE50914), // Premium WebOS Red
        scaffoldBackgroundColor: const Color(0xFF0F0F0F),
        cardTheme: CardThemeData(
          color: const Color(0xFF1A1A1A),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        ),
      ),
      home: const StorageDashboard(),
      debugShowCheckedModeBanner: false,
    );
  }
}

class StorageDashboard extends StatefulWidget {
  const StorageDashboard({Key? key}) : super(key: key);

  @override
  State<StorageDashboard> createState() => _StorageDashboardState();
}

class _StorageDashboardState extends State<StorageDashboard> {
  UploadTask? _uploadTask;
  String _status = "Ready to Test";
  double _progress = 0;
  
  final String _localFilePath = '/tmp/webos_test_big_file.bin';
  final String _remoteFilePath = 'webos_tests/large_file.bin';

  /// Generates a 5MB dummy file to allow enough time for Pause/Resume testing
  Future<File> _generateBigFile() async {
    setState(() => _status = "Generating 5MB file...");
    final file = File(_localFilePath);
    
    // Create 5MB of random data
    final bytes = Uint8List(5 * 1024 * 1024);
    await file.writeAsBytes(bytes);
    
    setState(() => _status = "File Generated at /tmp");
    return file;
  }

  void _startUpload() async {
    final file = await _generateBigFile();
    
    final ref = FirebaseStorage.instance.ref().child(_remoteFilePath);
    
    // Start Upload
    final task = ref.putFile(file);
    setState(() {
      _uploadTask = task;
      _status = "Uploading...";
    });

    // Listen to events
    task.snapshotEvents.listen((snapshot) {
      setState(() {
        _progress = snapshot.totalBytes > 0? snapshot.bytesTransferred / snapshot.totalBytes: 0;
        _status = "Status: ${snapshot.state.name}";
      });
      print('🔥 Storage State: ${snapshot.state} | Progress: ${(_progress * 100).toStringAsFixed(1)}%');
    }, onError: (e) {
      setState(() => _status = "Error: $e");
      print('❌ Upload Error: $e');
    });

    // Wait for completion
    try {
      await task;
      setState(() {
        _progress = 1.0;
        _status = "Upload Complete!";
      });
      print('✅ Upload Finished Successfully');

      // Fetch download URL after upload
      final ref = FirebaseStorage.instance.ref().child(_remoteFilePath);
      final url = await ref.getDownloadURL();
      setState(() {
        _status = "Upload Complete!\nDownload URL:\n$url";
      });
      print('🔗 Download URL: $url');
    } catch (e) {
      print('ℹ️ Task ended: $e');
    }
  }

  void _pauseTask() async {
    print('⏸️ Requesting Pause...');
    final result = await _uploadTask?.pause();
    print('⏸️ Pause result: $result');
  }

  void _resumeTask() async {
    print('▶️ Requesting Resume...');
    final result = await _uploadTask?.resume();
    print('▶️ Resume result: $result');
  }

  void _cancelTask() async {
    print('🛑 Requesting Cancel...');
    final result = await _uploadTask?.cancel();
    print('🛑 Cancel result: $result');
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('WebOS Firebase Storage'),
        backgroundColor: Colors.transparent,
        elevation: 0,
        centerTitle: true,
      ),
      body: Padding(
        padding: const EdgeInsets.all(24.0),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            // Status Card
            Card(
              child: Padding(
                padding: const EdgeInsets.all(20.0),
                child: Column(
                  children: [
                    Text(_status, style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.white70)),
                    const SizedBox(height: 20),
                    LinearProgressIndicator(
                      value: _progress,
                      backgroundColor: Colors.white10,
                      color: Theme.of(context).primaryColor,
                      minHeight: 8,
                    ),
                    const SizedBox(height: 10),
                    Text("${(_progress * 100).toStringAsFixed(1)}%", style: const TextStyle(color: Colors.white54)),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 40),
            
            // Action Buttons
            Wrap(
              spacing: 15,
              runSpacing: 15,
              alignment: WrapAlignment.center,
              children: [
                _ActionButton(label: "Start Upload", icon: Icons.cloud_upload, color: Colors.blue, onPressed: _startUpload),
                const Divider(height: 40, color: Colors.white10),
                _ActionButton(label: "Pause", icon: Icons.pause, color: Colors.orange, onPressed: _pauseTask),
                _ActionButton(label: "Resume", icon: Icons.play_arrow, color: Colors.purple, onPressed: _resumeTask),
                _ActionButton(label: "Cancel", icon: Icons.stop, color: Colors.red, onPressed: _cancelTask),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _ActionButton extends StatelessWidget {
  final String label;
  final IconData icon;
  final Color color;
  final VoidCallback onPressed;

  const _ActionButton({required this.label, required this.icon, required this.color, required this.onPressed});

  @override
  Widget build(BuildContext context) {
    return ElevatedButton.icon(
      onPressed: onPressed,
      icon: Icon(icon, size: 20),
      label: Text(label),
      style: ElevatedButton.styleFrom(
        backgroundColor: color.withOpacity(0.2),
        foregroundColor: color,
        padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 15),
        side: BorderSide(color: color.withOpacity(0.5)),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      ),
    );
  }
}


