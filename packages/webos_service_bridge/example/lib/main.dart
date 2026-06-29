import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'package:webos_service_bridge/webos_service_bridge.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'WebOS Service Bridge Demo',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: const DemoPage(),
    );
  }
}

class _SubHandle {
  WebOSServiceBridge? bridge;
  StreamSubscription<Map<String, dynamic>>? subscription;
  bool isActive = false;
}

class DemoPage extends StatefulWidget {
  const DemoPage({super.key});

  @override
  State<DemoPage> createState() => _DemoPageState();
}

class _DemoPageState extends State<DemoPage> {
  final List<String> _log = [];
  final _sub1 = _SubHandle();
  final _sub2 = _SubHandle();
  final _scrollController = ScrollController();

  static const _pretty = JsonEncoder.withIndent('  ');
  static const _labelStyle = TextStyle(fontSize: 18, color: Colors.grey);
  static const _headerStyle =
      TextStyle(fontSize: 22, fontWeight: FontWeight.bold);
  static final _btnStyle = ElevatedButton.styleFrom(
    textStyle: const TextStyle(fontSize: 18),
  );

  void _log_(String tag, Object? data) {
    setState(() {
      _log.clear();
      _log.add('[$tag] ${_pretty.convert(data)}');
    });
  }

  void _logErr(String tag, Object e) {
    final msg = e is PlatformException ? (e.message ?? '$e') : '$e';
    setState(() {
      _log.clear();
      _log.add('[$tag ERROR] $msg');
    });
  }

  Future<void> _getSystemSettings() async {
    try {
      final res = await WebOSServiceBridge.callOneReply(
        'luna://com.webos.settingsservice/getSystemSettings',
        payload: {'keys': ['localeInfo'], 'subscribe': false},
      );
      _log_('REQ2', res);
    } catch (e) {
      _logErr('REQ2', e);
    }
  }

  Future<void> _getSystemTime() async {
    try {
      final res = await WebOSServiceBridge.callOneReply(
        'luna://com.palm.systemservice/time/getSystemTime',
      );
      _log_('REQ1', res);
    } catch (e) {
      _logErr('REQ1', e);
    }
  }


  void _subscribe(
      _SubHandle handle, String tag, String uri, Map<String, dynamic> payload) {
    if (handle.isActive) return;
    handle.bridge = WebOSServiceBridge(uri, payload: payload);
    handle.subscription = handle.bridge!.subscribe().listen(
          (data) => _log_(tag, data),
          onError: (e) => _logErr(tag, e),
        );
    setState(() => handle.isActive = true);
  }

  Future<void> _cancel(_SubHandle handle, String tag) async {
    await handle.bridge?.cancel();
    await handle.subscription?.cancel();
    handle.bridge = null;
    handle.subscription = null;
    setState(() => handle.isActive = false);
    _log_(tag, {'cancelled': true});
  }

  @override
  void dispose() {
    _cancel(_sub1, 'SUB1');
    _cancel(_sub2, 'SUB2');

    _scrollController.dispose();
    super.dispose();
  }

  Widget _sectionHeader(String title) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 12, 16, 8),
      child: Text(title, style: _headerStyle),
    );
  }

  Widget _reqRow(String label, List<Widget> buttons) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
      child: Row(
        children: [
          Expanded(
            flex: 3,
            child: Text(label, style: _labelStyle),
          ),
          ...buttons,
        ],
      ),
    );
  }

  Widget _subRow(
    String label,
    String subLabel,
    String cancelLabel,
    _SubHandle handle,
    VoidCallback onSubscribe,
    VoidCallback onCancel,
  ) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
      child: Row(
        children: [
          Expanded(
            flex: 3,
            child: Text(label, style: _labelStyle),
          ),
          ElevatedButton(
            style: _btnStyle,
            onPressed: handle.isActive ? null : onSubscribe,
            child: Text(subLabel),
          ),
          const SizedBox(width: 8),
          ElevatedButton(
            style: _btnStyle,
            onPressed: handle.isActive ? onCancel : null,
            child: Text(cancelLabel),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Service Bridge Demo'),
        actions: [
          IconButton(
            icon: const Icon(Icons.clear_all),
            tooltip: 'Clear log',
            onPressed: () => setState(() => _log.clear()),
          ),
        ],
      ),
      body: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          _sectionHeader('Request'),
          _reqRow(
            'systemservice/time/getSystemTime',
            [
              ElevatedButton(
                style: _btnStyle,
                onPressed: _getSystemTime,
                child: const Text('Get'),
              ),
            ],
          ),
          _reqRow(
            'settingsservice/getSystemSettings',
            [
              ElevatedButton(
                style: _btnStyle,
                onPressed: _getSystemSettings,
                child: const Text('Get'),
              ),
            ],
          ),
          const Divider(height: 16),
          _sectionHeader('Subscribe'),
          _subRow(
            'connectionmanager/getStatus',
            'Subscribe 1',
            'Cancel 1',
            _sub1,
            () => _subscribe(
                _sub1,
                'SUB1',
                'luna://com.webos.service.connectionmanager/getStatus',
                {'subscribe': true}),
            () => _cancel(_sub1, 'SUB1'),
          ),
          _subRow(
            'connectionmanager/getStatus',
            'Subscribe 2',
            'Cancel 2',
            _sub2,
            () => _subscribe(
                _sub2,
                'SUB2',
                'luna://com.webos.service.connectionmanager/getStatus',
                {'subscribe': true}),
            () => _cancel(_sub2, 'SUB2'),
          ),
          const Divider(height: 8),
          _sectionHeader('Response'),
          Expanded(
            child: Container(
              margin: const EdgeInsets.fromLTRB(16, 0, 16, 16),
              decoration: BoxDecoration(
                border: Border.all(color: Colors.grey.shade400),
                borderRadius: BorderRadius.circular(8),
              ),
              child: ClipRRect(
                borderRadius: BorderRadius.circular(8),
                child: Scrollbar(
                  controller: _scrollController,
                  thumbVisibility: true,
                  child: ListView.builder(
                    controller: _scrollController,
                    padding: const EdgeInsets.all(12),
                    itemCount: _log.length,
                    itemBuilder: (_, i) => Text(
                      _log[i],
                      style: Theme.of(context)
                          .textTheme
                          .bodyLarge
                          ?.copyWith(fontFamily: 'monospace'),
                    ),
                  ),
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
