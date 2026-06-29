import 'package:flutter/material.dart';
import 'package:webos_display/webos_display.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> with WidgetsBindingObserver {
  final WebOSDisplay _display = WebOSDisplay();

  // Rotation info
  int? _rotationDegree;
  String? _rotationEvent;
  int _rotationEventCount = 0;

  // Panel bounds
  Size? _displayBounds;

  // Visibility state
  bool _isVisible = true;

  // Error message
  String? _lastError;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _display.addOrientationListener(_onOrientationChanged);
    _initPlatformState();
  }

  void _onOrientationChanged(int degree) {
    _rotationEventCount++;
    _rotationEvent = '[$_rotationEventCount] Orientation changed: $degree°';
    _fetchRotationDegree();
  }

  @override
  void didChangeMetrics() {
    super.didChangeMetrics();
    _fetchRotationDegree();
  }

  Future<void> _initPlatformState() async {
    if (!mounted) return;
    await Future.wait([_fetchRotationDegree(), _fetchDisplayBounds()]);
  }

  Future<void> _fetchRotationDegree() async {
    try {
      final degree = await _display.getRotationDegree();
      if (mounted) {
        setState(() {
          _rotationDegree = degree;
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _lastError = 'Error fetching rotation degree: $e';
        });
      }
    }
  }

  Future<void> _fetchDisplayBounds() async {
    try {
      final bounds = await _display.getBounds();
      if (mounted) {
        setState(() {
          _displayBounds = bounds;
          if (bounds == null) {
            _lastError = 'Display bounds: not available';
          }
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _lastError = 'Error fetching display bounds: $e';
        });
      }
    }
  }

  Future<void> _toggleVisible() async {
    final next = !_isVisible;
    await _display.setVisible(next);
    if (mounted) {
      setState(() => _isVisible = next);
    }
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _display.removeOrientationListener();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final mediaQuery = MediaQuery.of(context);

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
          title: const Text('WebOSDisplay example'),
          actions: [
            IconButton(
              icon: const Icon(Icons.refresh),
              onPressed: _initPlatformState,
              tooltip: 'Refresh',
            ),
          ],
        ),
        body: SingleChildScrollView(
          padding: const EdgeInsets.all(20.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _buildRotationCard(mediaQuery),
              const SizedBox(height: 12),
              _buildMediaQueryCard(mediaQuery),
              const SizedBox(height: 12),
              _buildDisplayBoundsCard(),
              if (_lastError != null) ...[
                const SizedBox(height: 12),
                _buildErrorCard(_lastError!),
              ],
              const SizedBox(height: 20),
              _buildVisibilityButton(),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildRotationCard(MediaQueryData mediaQuery) {
    final isPortrait = mediaQuery.orientation == Orientation.portrait;
    return _buildSectionCard(
      icon: isPortrait ? Icons.stay_current_portrait : Icons.stay_current_landscape,
      iconColor: isPortrait ? Colors.orange.shade800 : Colors.green.shade800,
      title: 'Rotation Info',
      subtitle: 'WebOSDisplay.getRotationDegree()',
      color: isPortrait ? Colors.orange.shade50 : Colors.green.shade50,
      children: [
        Text(
          isPortrait ? 'Portrait Mode' : 'Landscape Mode',
          style: TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.bold,
            color: isPortrait ? Colors.orange.shade800 : Colors.green.shade800,
          ),
        ),
        const SizedBox(height: 8),
        if (_rotationDegree != null) ...[
          _buildInfoRow('degree', '${_rotationDegree!}°'),
          _buildInfoRow('isPortrait', '${_rotationDegree == 90 || _rotationDegree == 270}'),
          _buildInfoRow('isLandscape', '${_rotationDegree == 0 || _rotationDegree == 180}'),
        ] else
          _buildPlaceholder('Loading...'),
        const Divider(height: 16),
        _buildSubtitle('Recent orientation event'),
        Text(
          _rotationEvent ?? 'No orientation events yet',
          style: TextStyle(
            color: _rotationEvent != null ? Colors.black : Colors.grey,
            fontWeight: FontWeight.w500,
            fontSize: 14,
          ),
        ),
      ],
    );
  }

  Widget _buildMediaQueryCard(MediaQueryData mediaQuery) {
    return _buildSectionCard(
      icon: Icons.phone_android,
      iconColor: Colors.blue.shade700,
      title: 'MediaQuery Info',
      subtitle: 'MediaQuery.of(context)',
      color: Colors.blue.shade50,
      children: [
        _buildInfoRow('.orientation', mediaQuery.orientation.toString()),
        _buildInfoRow('.size.width', '${mediaQuery.size.width.toStringAsFixed(1)} px'),
        _buildInfoRow('.size.height', '${mediaQuery.size.height.toStringAsFixed(1)} px'),
        _buildInfoRow('.devicePixelRatio', mediaQuery.devicePixelRatio.toStringAsFixed(2)),
      ],
    );
  }

  Widget _buildDisplayBoundsCard() {
    return _buildSectionCard(
      icon: Icons.tv,
      iconColor: Colors.teal.shade700,
      title: 'Display Bounds',
      subtitle: 'WebOSDisplay.getBounds()',
      color: Colors.teal.shade50,
      children: [
        if (_displayBounds != null) ...[
          _buildInfoRow('width', '${_displayBounds!.width.toInt()} px'),
          _buildInfoRow('height', '${_displayBounds!.height.toInt()} px'),
          _buildInfoRow('aspectRatio', _displayBounds!.aspectRatio.toStringAsFixed(3)),
          _buildInfoRow('isLandscape', '${_displayBounds!.width > _displayBounds!.height}'),
        ] else
          _buildPlaceholder('Not available'),
      ],
    );
  }

  Widget _buildVisibilityButton() {
    return Center(
      child: SizedBox(
        width: 300,
        height: 80,
        child: ElevatedButton.icon(
          onPressed: _toggleVisible,
          icon: Icon(_isVisible ? Icons.visibility_off : Icons.visibility),
          label: Text(_isVisible ? 'setVisible(false)' : 'setVisible(true)'),
          style: ElevatedButton.styleFrom(
            backgroundColor: _isVisible ? Colors.teal : Colors.grey,
            foregroundColor: Colors.white,
            textStyle: const TextStyle(fontSize: 16),
          ),
        ),
      ),
    );
  }

  Widget _buildSectionCard({
    required IconData icon,
    required Color iconColor,
    required String title,
    required String subtitle,
    required Color color,
    required List<Widget> children,
  }) {
    return Card(
      color: color,
      elevation: 1,
      child: Padding(
        padding: const EdgeInsets.all(14.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(icon, size: 28, color: iconColor),
                const SizedBox(width: 10),
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      title,
                      style: const TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.bold,
                        color: Colors.black87,
                      ),
                    ),
                    Text(
                      subtitle,
                      style: const TextStyle(fontSize: 14, color: Colors.black54),
                    ),
                  ],
                ),
              ],
            ),
            const Divider(height: 20),
            ...children,
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
          Text(
            value,
            style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.black, fontSize: 16),
          ),
        ],
      ),
    );
  }

  Widget _buildSubtitle(String text) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Text(text, style: const TextStyle(fontSize: 14, color: Colors.black54)),
    );
  }

  Widget _buildPlaceholder(String text) {
    return Text(text, style: const TextStyle(color: Colors.grey, fontSize: 16));
  }

  Widget _buildErrorCard(String error) {
    return Card(
      color: Colors.red.shade100,
      elevation: 1,
      child: Padding(
        padding: const EdgeInsets.all(12.0),
        child: Row(
          children: [
            Icon(Icons.error_outline, color: Colors.red.shade700, size: 24),
            const SizedBox(width: 10),
            Expanded(
              child: Text(
                error,
                style: TextStyle(color: Colors.red.shade900, fontSize: 14),
              ),
            ),
            IconButton(
              icon: Icon(Icons.close, color: Colors.red.shade700, size: 20),
              onPressed: () => setState(() => _lastError = null),
              padding: EdgeInsets.zero,
              constraints: const BoxConstraints(),
            ),
          ],
        ),
      ),
    );
  }
}
