import 'firebase_auth_webos.dart';
import 'src/messages.g.dart' as msgs;
import 'src/firebase_user_webos.dart';

class FirebaseAuthFlutterApiImpl extends msgs.FirebaseAuthFlutterApi {
  @override
  Future<void> onAuthStateChanged(
      String appName,
      msgs.PigeonUserDetails? user) async {

    final auth = FirebaseAuthWebos.getInstance(appName);

    if (auth == null) {
      return;
    }

    if (user == null) {
      auth.notifyAuthStateChange(null);
    } else {
      auth.notifyAuthStateChange(FirebaseUserWebos(auth, user));
    }
  }

  @override
  Future<void> onIdTokenChanged(
      String appName,
      msgs.PigeonUserDetails? user) async {

    final auth = FirebaseAuthWebos.getInstance(appName);
    if (auth == null) return;

    if (user == null) {
      auth.notifyIdTokenChange(null);
    } else {
      auth.notifyIdTokenChange(FirebaseUserWebos(auth, user));
    }
  }

  @override
  Future<void> onUserChanged(
      String appName,
      msgs.PigeonUserDetails? user) async {

    final auth = FirebaseAuthWebos.getInstance(appName);
    if (auth == null) return;

    if (user == null) {
      auth.notifyUserChange(null);
    } else {
      auth.notifyUserChange(FirebaseUserWebos(auth, user));
    }
  }

}