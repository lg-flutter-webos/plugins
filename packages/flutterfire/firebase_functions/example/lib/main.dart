import 'package:cloud_functions/cloud_functions.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:flutter/material.dart';

import 'firebase_options.dart';

Future<void> main() async {
  print('[main] Starting application...');
  WidgetsFlutterBinding.ensureInitialized();

  await Firebase.initializeApp(
    options: DefaultFirebaseOptions.currentPlatform,
  );

  print('[main] Firebase initialized');

  //FirebaseFunctions.instance.useFunctionsEmulator('localhost', 5001);
  //print('[main] Functions emulator configured');

  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      home: const HomePage(),
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  bool _loading = false;

  Future<void> _callFunction() async {
    print('[MyApp] _callFunction() called');

    setState(() => _loading = true);

    try {
      final callable = FirebaseFunctions.instance.httpsCallable(
        'validateCoupon',
        options: HttpsCallableOptions(
          timeout: const Duration(seconds: 5),
        ),
      );

      final parameters = {
        'coupon': 'summer2025',
      };

      print('[MyApp] Calling function with: $parameters');

      final result = await callable(parameters);

      print('[MyApp] Raw result: $result');

      final bool isValid =
          result.data.toString().trim().toLowerCase() == "true";

      print('[MyApp] Parsed isValid: $isValid');

      if (!mounted) return;

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Coupon valid: $isValid'),
          backgroundColor: isValid ? Colors.green : Colors.red,
        ),
      );
    } catch (e) {
      print('[MyApp] Error: $e');

      if (!mounted) return;

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Error: $e'),
          backgroundColor: Colors.red,
        ),
      );
    } finally {
      if (mounted) {
        setState(() => _loading = false);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    print('[MyApp] Building UI');

    return Scaffold(
      appBar: AppBar(
        title: const Text('Firebase Functions webOS test'),
      ),
      body: Center(
        child: _loading
            ? const CircularProgressIndicator()
            : ElevatedButton(
                onPressed: _callFunction,
                child: const Text('Validate Coupon'),
              ),
      ),
    );
  }
}