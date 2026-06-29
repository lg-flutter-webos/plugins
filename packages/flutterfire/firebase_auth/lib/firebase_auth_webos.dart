import 'package:firebase_auth_platform_interface/firebase_auth_platform_interface.dart';
import 'package:firebase_auth_platform_interface/src/action_code_settings.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:flutter/services.dart' show PlatformException;

import 'src/firebase_user_webos.dart';
import 'src/messages.g.dart' as msgs;

import 'dart:async';
import 'firebase_auth_flutter_api_impl.dart';

// ignore_for_file: require_trailing_commas

class FirebaseAuthWebos extends FirebaseAuthPlatform {
  static final msgs.FirebaseAuthHostApi _api = msgs.FirebaseAuthHostApi();
  static final Map<String, FirebaseAuthWebos> _instances = {};
  
  /// Get an existing FirebaseAuthWebos instance by app name
  static FirebaseAuthWebos? getInstance(String appName) {
    return _instances[appName];
  }

  static void registerWith() {
    FirebaseAuthPlatform.instance = _BootstrapFirebaseAuthWebos();
  }

  final FirebaseApp app;
  FirebaseAuthWebos._(this.app) : super() {
  msgs.FirebaseAuthFlutterApi.setup(
    FirebaseAuthFlutterApiImpl(),
  );
}

  final StreamController<UserPlatform?> _authStateController =
      StreamController<UserPlatform?>.broadcast();

  final StreamController<UserPlatform?> _idTokenController =
      StreamController<UserPlatform?>.broadcast();

  final StreamController<UserPlatform?> _userChangesController =
      StreamController<UserPlatform?>.broadcast();

  void _emitAuthState(UserPlatform? user) {
  _currentUser = user;
  _authStateController.add(user);
}

  void _emitIdToken(UserPlatform? user) {
  _currentUser = user;
  _idTokenController.add(user);
}

  void _emitUserChanges(UserPlatform? user) {
    _currentUser = user;
    _userChangesController.add(user);
}
  
  void notifyAuthStateChange(UserPlatform? user) {
    _emitAuthState(user);
  }

  void notifyIdTokenChange(UserPlatform? user) {
    _emitIdToken(user);
  }

  void notifyUserChange(UserPlatform? user) {
    _emitUserChanges(user);
  }

  @override
  Stream<UserPlatform?> authStateChanges() {
    return _authStateController.stream;
}

  @override
  Stream<UserPlatform?> idTokenChanges() {
    return _idTokenController.stream;
}

  @override
  Stream<UserPlatform?> userChanges() {
    return _userChangesController.stream;
}

  @override
  FirebaseAuthPlatform delegateFor({
    required FirebaseApp app,
  }) {
    return FirebaseAuthWebos.instanceFor(
      app: app,
      pluginConstants: {},
    );
  }

  @override
  factory FirebaseAuthWebos.instanceFor({
    required FirebaseApp app,
    required Map<String, dynamic> pluginConstants,
  }) {
    return _instances.putIfAbsent(
      app.name,
      () => FirebaseAuthWebos._(app),
    );
  }

  @override
  FirebaseAuthPlatform setInitialValues({
    PigeonUserDetails? currentUser,
    String? languageCode,
  }) {
    if (currentUser != null) {
      final converted = msgs.PigeonUserDetails(
        userInfo: msgs.PigeonUserInfo(
          uid: currentUser.userInfo.uid,
          email: currentUser.userInfo.email,
          displayName: currentUser.userInfo.displayName,
          phoneNumber: currentUser.userInfo.phoneNumber,
          photoUrl: currentUser.userInfo.photoUrl,
          providerId: currentUser.userInfo.providerId ?? '',
          isEmailVerified: currentUser.userInfo.isEmailVerified,
        ),
        providerData: [],
      );

      _currentUser = FirebaseUserWebos(this, converted);
    }

    return this;
  }

  @override
  set currentUser(UserPlatform? value) {
    _currentUser = value;
  }

  UserPlatform? _currentUser;

  @override
  UserPlatform? get currentUser => _currentUser;

  @override
    Future<UserCredentialPlatform> signInWithEmailAndPassword(
        String email, String password) async {
      try {
        final details = await _api.signInWithEmailAndPassword(
            app.name, email, password);

        final user = FirebaseUserWebos(this, details);
        _currentUser = user;

        return WebosUserCredential(
          auth: this,
          credential: null,
          additionalUserInfo: AdditionalUserInfo(
            isNewUser: details.userInfo.providerId == 'anonymous',
            profile: {},
            providerId: details.userInfo.providerId,
            username: details.userInfo.displayName,
            authorizationCode: null,
          ),
          user: user,
        );
      } catch (e) {
        throw _convertException(e);
      }
    }

  @override
  Future<UserCredentialPlatform> signInWithCustomToken(
      String token) async {
    try {
      final details =
          await _api.signInWithCustomToken(app.name, token);

      final user = FirebaseUserWebos(this, details);
      _currentUser = user;

      return WebosUserCredential(
        auth: this,
        credential: null,
        additionalUserInfo: AdditionalUserInfo(
          isNewUser: false,
          profile: {},
          providerId: details.userInfo.providerId,
          username: details.userInfo.displayName,
          authorizationCode: null,
        ),
        user: user,
      );
    } catch (e) {
      throw _convertException(e);
    }
  }

  @override
  Future<UserCredentialPlatform> createUserWithEmailAndPassword(
      String email, String password) async {
    try {
      final details = await _api.createUserWithEmailAndPassword(
          app.name, email, password);

      final user = FirebaseUserWebos(this, details);
      _currentUser = user;

      return WebosUserCredential(
        auth: this,
        credential: null,
        additionalUserInfo: AdditionalUserInfo(
          isNewUser: true,
          profile: {},
          providerId: details.userInfo.providerId,
          username: details.userInfo.displayName,
          authorizationCode: null,
        ),
        user: user,
      );
    } catch (e) {
      throw _convertException(e);
    }
  }

  @override
  Future<UserCredentialPlatform> signInAnonymously() async {
    try {
      final details = await _api.signInAnonymously(app.name);

      final user = FirebaseUserWebos(this, details);
      _currentUser = user;

      return WebosUserCredential(
        auth: this,
        credential: null,
        additionalUserInfo: AdditionalUserInfo(
          isNewUser: true,
          profile: {},
          providerId: details.userInfo.providerId,
          username: details.userInfo.displayName,
          authorizationCode: null,
        ),
        user: user,
      );
    } catch (e) {
      throw _convertException(e);
    }
  }

  @override
  Future<void> signOut() async {
    await _api.signOut(app.name);
    _currentUser = null;
  }

  @override
  Future<String> getIdToken(bool forceRefresh) async {
    try {
      return await _api.getIdToken(
        app.name,
        forceRefresh,
      );
    } catch (e) {
      throw _convertException(e);
    }
  }

  @override
  Future<void> sendPasswordResetEmail(
    String email, [
    ActionCodeSettings? actionCodeSettings,
  ]) async {
    try {
      await _api.sendPasswordResetEmail(app.name, email);
    } catch (e) {
      throw _convertException(e);
    }
  }

  @override
  Future<void> useAuthEmulator(String host, int port) async {
    try {
      await _api.useAuthEmulator(app.name, host, port);
    } catch (e) {
      throw _convertException(e);
    }
  }

  Exception _convertException(dynamic exception) {
    if (exception is PlatformException) {
      return FirebaseAuthException(
        code: exception.code,
        message: exception.message,
      );
    }
    return exception as Exception;
}
}

class WebosUserCredential extends UserCredentialPlatform {
  WebosUserCredential({
    required FirebaseAuthPlatform auth,
    required AuthCredential? credential,
    required AdditionalUserInfo? additionalUserInfo,
    required UserPlatform? user,
  }) : super(
          auth: auth,
          credential: credential,
          additionalUserInfo: additionalUserInfo,
          user: user,
        );
}

class _BootstrapFirebaseAuthWebos extends FirebaseAuthPlatform {
  @override
  FirebaseAuthPlatform delegateFor({
    required FirebaseApp app,
  }) {
    return FirebaseAuthWebos.instanceFor(
      app: app,
      pluginConstants: {},
    );
  }
}
