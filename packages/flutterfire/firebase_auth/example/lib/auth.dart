
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';

import 'main.dart';

class ScaffoldSnackbar {
  ScaffoldSnackbar(this._context);

  factory ScaffoldSnackbar.of(BuildContext context) {
    return ScaffoldSnackbar(context);
  }

  final BuildContext _context;

  void show(String message) {
    ScaffoldMessenger.of(_context)
      ..hideCurrentSnackBar()
      ..showSnackBar(
        SnackBar(
          content: Text(message),
          behavior: SnackBarBehavior.floating,
        ),
      );
  }
}

enum AuthMode { login, register, token }

extension on AuthMode {
  String get label {
    switch (this) {
      case AuthMode.login:
        return 'Sign in';
      case AuthMode.register:
        return 'Register';
      case AuthMode.token:
        return 'Sign in with Token';
    }
  }
}

class AuthGate extends StatefulWidget {
  const AuthGate({Key? key}) : super(key: key);

  @override
  State<StatefulWidget> createState() => _AuthGateState();
}

class _AuthGateState extends State<AuthGate> {
  final TextEditingController emailController = TextEditingController();
  final TextEditingController passwordController = TextEditingController();
  final TextEditingController tokenController = TextEditingController();
  final GlobalKey<FormState> formKey = GlobalKey<FormState>();

  AuthMode mode = AuthMode.login;
  String error = '';
  bool isLoading = false;

  void setIsLoading() {
    setState(() {
      isLoading = !isLoading;
    });
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: FocusScope.of(context).unfocus,
      child: Scaffold(
        body: Center(
          child: SingleChildScrollView(
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 20),
              child: SafeArea(
                child: Form(
                  key: formKey,
                  autovalidateMode: AutovalidateMode.onUserInteraction,
                  child: ConstrainedBox(
                    constraints: const BoxConstraints(maxWidth: 400),
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        // Error display
                        if (error.isNotEmpty)
                          Container(
                            padding: const EdgeInsets.all(10),
                            margin: const EdgeInsets.only(bottom: 20),
                            color: Colors.red,
                            child: Text(
                              error,
                              style: const TextStyle(color: Colors.white),
                            ),
                          ),

                        // Email/Password fields
                        if (mode != AuthMode.token) ...[
                          TextFormField(
                            controller: emailController,
                            decoration: const InputDecoration(
                              hintText: 'Email',
                              border: OutlineInputBorder(),
                            ),
                            keyboardType: TextInputType.emailAddress,
                            validator: (value) =>
                                value != null && value.isNotEmpty ? null : 'Required',
                          ),
                          const SizedBox(height: 20),
                          TextFormField(
                            controller: passwordController,
                            obscureText: true,
                            decoration: const InputDecoration(
                              hintText: 'Password',
                              border: OutlineInputBorder(),
                            ),
                            validator: (value) =>
                                value != null && value.isNotEmpty ? null : 'Required',
                          ),
                        ],

                        // Token field
                        if (mode == AuthMode.token) ...[
                          TextFormField(
                            controller: tokenController,
                            decoration: const InputDecoration(
                              hintText: 'Custom Token',
                              border: OutlineInputBorder(),
                            ),
                            maxLines: 3,
                            validator: (value) =>
                                value != null && value.isNotEmpty ? null : 'Required',
                          ),
                        ],

                        const SizedBox(height: 20),

                        // Submit button
                        SizedBox(
                          width: double.infinity,
                          height: 50,
                          child: ElevatedButton(
                            onPressed: isLoading ? null : _submit,
                            child: isLoading
                                ? const CircularProgressIndicator()
                                : Text(mode.label),
                          ),
                        ),

                        // Forgot password
                        if (mode != AuthMode.token)
                          TextButton(
                            onPressed: _resetPassword,
                            child: const Text('Forgot password?'),
                          ),

                        // Mode switcher
                        const SizedBox(height: 20),
                        if (mode != AuthMode.token)
                          RichText(
                            text: TextSpan(
                              style: Theme.of(context).textTheme.bodyLarge,
                              children: [
                                TextSpan(
                                  text: mode == AuthMode.login
                                      ? "Don't have an account? "
                                      : "Already have an account? ",
                                ),
                                TextSpan(
                                  text: mode == AuthMode.login ? 'Register' : 'Login',
                                  style: const TextStyle(color: Colors.blue),
                                  recognizer: TapGestureRecognizer()
                                    ..onTap = () {
                                      setState(() {
                                        mode = mode == AuthMode.login
                                            ? AuthMode.register
                                            : AuthMode.login;
                                      });
                                    },
                                ),
                              ],
                            ),
                          ),

                        // Token login option
                        const SizedBox(height: 10),
                        RichText(
                          text: TextSpan(
                            style: Theme.of(context).textTheme.bodyLarge,
                            children: [
                              const TextSpan(text: 'Or '),
                              TextSpan(
                                text: mode == AuthMode.token
                                    ? 'Use email/password'
                                    : 'Sign in with custom token',
                                style: const TextStyle(color: Colors.blue),
                                recognizer: TapGestureRecognizer()
                                  ..onTap = () {
                                    setState(() {
                                      mode = mode == AuthMode.token
                                          ? AuthMode.login
                                          : AuthMode.token;
                                    });
                                  },
                              ),
                            ],
                          ),
                        ),

                        // Anonymous login
                        const SizedBox(height: 10),
                        TextButton(
                          onPressed: _anonymousLogin,
                          child: const Text("Continue as Guest"),
                        ),
                      ],
                    ),
                  ),
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }

  Future<void> _submit() async {
    if (!(formKey.currentState?.validate() ?? false)) return;

    setIsLoading();
    setState(() => error = '');

    try {
      switch (mode) {
        case AuthMode.login:
          await auth.signInWithEmailAndPassword(
            email: emailController.text.trim(),
            password: passwordController.text.trim(),
          );
          break;
        case AuthMode.register:
          await auth.createUserWithEmailAndPassword(
            email: emailController.text.trim(),
            password: passwordController.text.trim(),
          );
          break;
        case AuthMode.token:
          await auth.signInWithCustomToken(tokenController.text.trim());
          break;
      }
    } on FirebaseAuthException catch (e) {
      setState(() => error = e.message ?? 'Authentication error');
    } catch (e) {
      setState(() => error = e.toString());
    }

    setIsLoading();
  }

  Future<void> _anonymousLogin() async {
    setIsLoading();
    setState(() => error = '');

    try {
      await auth.signInAnonymously();
    } on FirebaseAuthException catch (e) {
      setState(() => error = e.message ?? 'Authentication error');
    } catch (e) {
      setState(() => error = e.toString());
    }

    setIsLoading();
  }

  Future<void> _resetPassword() async {
    if (emailController.text.isEmpty) {
      setState(() => error = "Enter email first");
      return;
    }

    try {
      await auth.sendPasswordResetEmail(email: emailController.text.trim());
      setState(() => error = "Password reset email sent");
    } on FirebaseAuthException catch (e) {
      setState(() => error = e.message ?? 'Error sending reset email');
    } catch (e) {
      setState(() => error = e.toString());
    }
  }
}