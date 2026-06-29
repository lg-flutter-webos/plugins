import 'package:flutter/material.dart';
import 'package:webos_app_manager/webos_app_manager.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _QueryResult {
  final String appId;
  final AppLoadStatus? status;
  final String? error;
  final bool loading;

  const _QueryResult({
    required this.appId,
    this.status,
    this.error,
    this.loading = false,
  });

  _QueryResult copyWith({
    AppLoadStatus? status,
    String? error,
    bool? loading,
  }) =>
      _QueryResult(
        appId: appId,
        status: status ?? this.status,
        error: error ?? this.error,
        loading: loading ?? this.loading,
      );
}

class _MyAppState extends State<MyApp> {
  final WebOSAppManager _appManager = WebOSAppManager();

  static const _presets = [
    'com.flutter.app.webos-app-manager-example',
    'com.example.nonexistent.app',
  ];

  late List<_QueryResult> _results = _presets
      .map((id) => _QueryResult(appId: id, loading: true))
      .toList();

  @override
  void initState() {
    super.initState();
    _fetchAll();
  }

  Future<void> _fetchAll() async {
    setState(() {
      _results = _presets
          .map((id) => _QueryResult(appId: id, loading: true))
          .toList();
    });

    for (var i = 0; i < _presets.length; i++) {
      _fetchOne(i);
    }
  }

  Future<void> _fetchOne(int index) async {
    final appId = _presets[index];
    try {
      final result = await _appManager.getAppLoadStatus(appId);
      if (!mounted) return;
      setState(() {
        _results[index] = _results[index].copyWith(
          status: result,
          loading: false,
          error: result == null ? 'getAppLoadStatus: not available' : null,
        );
      });
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _results[index] = _results[index].copyWith(
          loading: false,
          error: 'Error: $e',
        );
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.teal),
        scaffoldBackgroundColor: Colors.grey.shade100,
        useMaterial3: true,
      ),
      home: Scaffold(
        appBar: AppBar(
          backgroundColor: Colors.teal,
          foregroundColor: Colors.white,
          title: const Text('WebOSAppManager example'),
          actions: [
            IconButton(
              icon: const Icon(Icons.refresh),
              onPressed: _fetchAll,
              tooltip: 'Refresh',
            ),
          ],
        ),
        body: SingleChildScrollView(
          padding: const EdgeInsets.all(20.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              for (final r in _results) ...[
                _buildStatusCard(r),
                const SizedBox(height: 16),
              ],
            ],
          ),
        ),
      ),
    );
  }

  MaterialColor _colorFor(_QueryResult r) {
    if (r.loading) return Colors.teal;
    if (r.error != null) return Colors.red;
    if (r.status?.exist == true) return Colors.green;
    return Colors.orange;
  }

  IconData _iconFor(_QueryResult r) {
    if (r.loading) return Icons.hourglass_empty;
    if (r.error != null) return Icons.error_outline;
    if (r.status?.exist == true) return Icons.check_circle_outline;
    return Icons.cancel_outlined;
  }

  Widget _buildStatusCard(_QueryResult r) {
    final baseColor = _colorFor(r);
    final icon = _iconFor(r);

    return Card(
      color: baseColor.shade50,
      elevation: 1,
      child: Padding(
        padding: const EdgeInsets.all(14.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(icon, size: 28, color: baseColor.shade700),
                const SizedBox(width: 10),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'getAppLoadStatus',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.bold,
                          color: Colors.black87,
                        ),
                      ),
                      Text(
                        r.appId,
                        style: TextStyle(fontSize: 13, color: Colors.black54),
                        overflow: TextOverflow.ellipsis,
                      ),
                    ],
                  ),
                ),
              ],
            ),
            const Divider(height: 20),
            if (r.loading)
              const Center(child: CircularProgressIndicator())
            else if (r.error != null)
              Text(r.error!, style: TextStyle(color: Colors.red.shade700, fontSize: 14))
            else ...[
              _buildInfoRow('appId', r.appId),
              _buildInfoRow('exist', r.status!.exist.toString()),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildInfoRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 3),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(color: Colors.black87, fontSize: 16)),
          Flexible(
            child: Text(
              value,
              style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.black, fontSize: 16),
              textAlign: TextAlign.end,
              overflow: TextOverflow.ellipsis,
            ),
          ),
        ],
      ),
    );
  }
}
