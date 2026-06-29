#pragma once

#include <flutter/plugin_registrar.h>
#include <firebase/auth.h>
#include <memory>
#include <string>

#include "messages.g.h"

namespace firebase_auth_webos {

class AuthStateListenerImpl;
class IdTokenListenerImpl;

class FirebaseAuthWebosPlugin : public flutter::Plugin,
                                public FirebaseAuthHostApi {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar);

  static void FirebaseAuthWebosPluginRegisterWithRegistrar(
      FlutterDesktopPluginRegistrarRef registrar);

  FirebaseAuthWebosPlugin();
  virtual ~FirebaseAuthWebosPlugin();

  FirebaseAuthWebosPlugin(const FirebaseAuthWebosPlugin&) = delete;
  FirebaseAuthWebosPlugin& operator=(const FirebaseAuthWebosPlugin&) = delete;

  void SignInWithEmailAndPassword(const std::string& app_name,
    const std::string& email, const std::string& password,
    std::function<void(ErrorOr<PigeonUserDetails>)> result) override;

  void SignInWithCustomToken(
    const std::string& app_name,
    const std::string& token,
    std::function<void(ErrorOr<PigeonUserDetails>)> result) override;

  void GetIdToken(const std::string& app_name,bool force_refresh,
    std::function<void(ErrorOr<std::string>)> result) override;

  void CreateUserWithEmailAndPassword(const std::string& app_name,
    const std::string& email, const std::string& password,
    std::function<void(ErrorOr<PigeonUserDetails>)> result) override;

  void SignInAnonymously(const std::string& app_name,
    std::function<void(ErrorOr<PigeonUserDetails>)> result) override;

  void SignOut(const std::string& app_name,
    std::function<void(std::optional<FlutterError>)> result) override;

  void UpdateUserProfile(const std::string& app_name,
    const std::string* display_name,
    const std::string* photo_url,
    std::function<void(std::optional<FlutterError>)> result) override;

  void SendEmailVerification(const std::string& app_name,
    std::function<void(std::optional<FlutterError>)> result) override;

  void SendPasswordResetEmail(const std::string& app_name,
    const std::string& email,
    std::function<void(std::optional<FlutterError>)> result) override;

  void UpdatePassword(const std::string& app_name,
    const std::string& new_password,
    std::function<void(std::optional<FlutterError>)> result) override;

  void Reload(const std::string& app_name,
    std::function<void(ErrorOr<PigeonUserDetails>)> result) override;

  void Delete(const std::string& app_name,
    std::function<void(std::optional<FlutterError>)> result) override;

  void UseAuthEmulator(const std::string& app_name,
    const std::string& host,
    int64_t port,
    std::function<void(std::optional<FlutterError>)> result) override;

  static PigeonUserDetails UserToPigeon(const firebase::auth::User& user);

 private:

  flutter::BinaryMessenger* messenger_;

  std::map<std::string, std::unique_ptr<AuthStateListenerImpl>> auth_listeners_;
  std::map<std::string, std::unique_ptr<IdTokenListenerImpl>> token_listeners_;
  friend class AuthStateListenerImpl;

  firebase::auth::Auth* GetAuth(const std::string& app_name);

  template <typename T>
  void HandleAuthFuture(firebase::Future<T> future, std::function<void(ErrorOr<PigeonUserDetails>)> result);

  void HandleVoidFuture(firebase::Future<void> future, std::function<void(std::optional<FlutterError>)> result);

  static std::string AuthErrorString(firebase::auth::AuthError err);
};

}  // namespace firebase_auth_webos