import 'package:pigeon/pigeon.dart';

@ConfigurePigeon(PigeonOptions(
  dartOut: 'lib/src/messages.g.dart',
  cppHeaderOut: 'webos/messages.g.h',
  cppSourceOut: 'webos/messages.g.cc',
  cppOptions: CppOptions(namespace: 'firebase_auth_webos'),
  dartPackageName: 'firebase_auth_webos',
))

// ---------------------------------------------------------------------------
// Data classes
// ---------------------------------------------------------------------------

/// User info from a provider (e.g. Google, email)
class PigeonUserInfo {
  PigeonUserInfo({
    required this.uid,
    required this.providerId,
    this.email,
    this.displayName,
    this.photoUrl,
    this.phoneNumber,
    this.isEmailVerified = false,
    this.isAnonymous = false,
  });
  String uid;
  String providerId;
  String? email;
  String? displayName;
  String? photoUrl;
  String? phoneNumber;
  bool isEmailVerified;
  bool isAnonymous;
}

/// Core user details returned for every auth operation
class PigeonUserDetails {
  PigeonUserDetails({
    required this.userInfo,
    required this.providerData,
  });
  PigeonUserInfo userInfo;
  List<PigeonUserInfo?> providerData;
}

// ---------------------------------------------------------------------------
// Host API — Dart → C++
// ---------------------------------------------------------------------------

@HostApi()
abstract class FirebaseAuthHostApi {
  /// Sign in with email + password
  @async
  PigeonUserDetails signInWithEmailAndPassword(
      String appName, String email, String password);

  /// Create account with email + password
  @async
  PigeonUserDetails createUserWithEmailAndPassword(
      String appName, String email, String password);

  /// Sign in anonymously
  @async
  PigeonUserDetails signInAnonymously(String appName);

  /// Sign out the current user
  @async
  void signOut(String appName);

  @async
  PigeonUserDetails signInWithCustomToken(
    String appName,
    String token,
  );

  @async
  String getIdToken(
    String appName,
    bool forceRefresh,
  );

  /// Update user profile (display name and/or photo URL)
  @async
  void updateUserProfile(
    String appName,
    String? displayName,
    String? photoUrl,
  );

  /// Send email verification to current user
  @async
  void sendEmailVerification(String appName);

  /// Send password reset email
  @async
  void sendPasswordResetEmail(String appName, String email);

  /// Update user password
  @async
  void updatePassword(String appName, String newPassword);

  /// Reload user data from server
  @async
  PigeonUserDetails reload(String appName);

  /// Delete user account
  @async
  void delete(String appName);

  /// Configure Firebase Auth emulator
  @async
  void useAuthEmulator(String appName, String host, int port);
}

@FlutterApi()
abstract class FirebaseAuthFlutterApi {
  void onAuthStateChanged(String appName, PigeonUserDetails? user);

  void onIdTokenChanged(String appName, PigeonUserDetails? user);

  void onUserChanged(String appName, PigeonUserDetails? user);
}