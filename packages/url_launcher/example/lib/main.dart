import 'dart:async';
import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'URL Launcher',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: const MyHomePage(title: 'URL Launcher'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});
  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  Future<void>? _launched;

  Future<void> _launch(Uri url) async {
    if (!await launchUrl(url)) {
      throw Exception('Could not launch $url');
    }
  }

  Widget _launchStatus(BuildContext context, AsyncSnapshot<void> snapshot) {
    if (snapshot.hasError) {
      return Text('Error: ${snapshot.error}');
    } else {
      return const Text('');
    }
  }

  @override
  Widget build(BuildContext context) {
    final Uri httpsUrl = Uri.parse('https://www.cylog.org/headers/');
    final Uri httpUrl = Uri.parse('http://www.lge.com');
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: ListView(
        children: <Widget>[
          Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: <Widget>[
              const Padding(
                padding: EdgeInsets.all(16.0),
                child: Text('https://www.cylog.org/headers/'),
              ),
              ElevatedButton(
                onPressed: () => setState(() {
                  _launched = _launch(httpsUrl);
                }),
                child: const Text('Launch HTTPS URL'),
              ),
              const Padding(padding: EdgeInsets.all(32.0)),
              const Padding(
                padding: EdgeInsets.all(16.0),
                child: Text('http://www.lge.com'),
              ),
              ElevatedButton(
                onPressed: () => setState(() {
                  _launched = _launch(httpUrl);
                }),
                child: const Text('Launch HTTP URL'),
              ),
              const Padding(padding: EdgeInsets.all(32.0)),
              FutureBuilder<void>(future: _launched, builder: _launchStatus),
            ],
          ),
        ],
      ),
    );
  }
}
