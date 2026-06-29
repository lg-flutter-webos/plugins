// ignore_for_file: require_trailing_comma
import 'package:firebase_auth_platform_interface/firebase_auth_platform_interface.dart';
import 'package:firebase_auth_platform_interface/src/action_code_settings.dart';
import 'package:flutter/services.dart' show PlatformException;

import 'messages.g.dart' as msgs;
import '../firebase_auth_webos.dart';

class _NoopMultiFactor extends MultiFactorPlatform {
  _NoopMultiFactor(FirebaseAuthPlatform auth) : super(auth);
}

PigeonUserDetails _toPlatformPigeon(msgs.PigeonUserDetails d) {
  final info = d.userInfo;
  
  return PigeonUserDetails(
    userInfo: PigeonUserInfo(
      uid: info.uid,
      email: info.email,
      displayName: info.displayName,
      phoneNumber: info.phoneNumber,
      photoUrl: info.photoUrl,
      providerId: info.providerId,
      isAnonymous: info.isAnonymous,
      isEmailVerified: info.isEmailVerified,
      creationTimestamp: null,
      lastSignInTimestamp: null,
      refreshToken: null,
      tenantId: null,
    ),
    providerData: [],
  );
}

class FirebaseUserWebos extends UserPlatform {
  final FirebaseAuthWebos _auth;
  final msgs.FirebaseAuthHostApi _api = msgs.FirebaseAuthHostApi();

  FirebaseUserWebos(this._auth, msgs.PigeonUserDetails details)
      : super(
          _auth,
          _NoopMultiFactor(_auth),
          _toPlatformPigeon(details),
        );

  @override
  Future<String> getIdToken(bool forceRefresh) async {
    try {
      return await _auth.getIdToken(forceRefresh);
    } catch (e) {
      throw _convertException(e);
    }
  }

  @override
  Future<void> updateProfile(Map<String, String?> profile) async {
    try {
      await _api.updateUserProfile(
        _auth.app.name,
        profile['displayName'],
        profile['photoURL'],
      );
    } catch (e) {
      throw _convertException(e);
    }
  }

  @override
  Future<void> sendEmailVerification([
    ActionCodeSettings? actionCodeSettings,
  ]) async {
    try {
      await _api.sendEmailVerification(_auth.app.name);
    } catch (e) {
      throw _convertException(e);
    }
  }

  @override
  Future<void> reload() async {
    try {
      final details = await _api.reload(_auth.app.name);
      _auth.notifyUserChange(FirebaseUserWebos(_auth, details));
    } catch (e) {
      throw _convertException(e);
    }
  }

  @override
  Future<void> delete() async {
    try {
      await _api.delete(_auth.app.name);
    } catch (e) {
      throw _convertException(e);
    }
  }

  @override
  Future<void> updatePassword(String newPassword) async {
    try {
      await _api.updatePassword(_auth.app.name, newPassword);
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