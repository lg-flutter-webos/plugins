#include "firebase_auth_webos_plugin.h"
#include "firebase_core_webos/firebase_app_holder.h"
#include "firebase_auth_webos/firebase_auth_webos_plugin.h"

#include <firebase/auth.h>
#include <firebase/app.h>
#include <flutter/plugin_registrar.h>
#include <optional>
#include <map>

namespace firebase_auth_webos {

using namespace firebase::auth;

class AuthStateListenerImpl : public firebase::auth::AuthStateListener {
  public:
  AuthStateListenerImpl(
      flutter::BinaryMessenger* messenger,
      const std::string& app_name)
      : flutter_api_(messenger), app_name_(app_name) {}

    void OnAuthStateChanged(firebase::auth::Auth* auth) override {
      auto user = auth->current_user();

      if (user.is_valid()) {
        auto pigeon = FirebaseAuthWebosPlugin::UserToPigeon(user);

        flutter_api_.OnAuthStateChanged(
            app_name_,
            &pigeon,
            []() {},
            [](const FlutterError& error) {});

        flutter_api_.OnUserChanged(
            app_name_,
            &pigeon,
            []() {},
            [](const FlutterError& error) {});
      } else {
        flutter_api_.OnAuthStateChanged(
            app_name_,
            nullptr,
            []() {},
            [](const FlutterError& error) {});

        flutter_api_.OnUserChanged(
            app_name_,
            nullptr,
            []() {},
            [](const FlutterError& error) {});
      }
    }

 private:
  FirebaseAuthFlutterApi flutter_api_;
  std::string app_name_;
};

class IdTokenListenerImpl : public firebase::auth::IdTokenListener {
 public:
  IdTokenListenerImpl(flutter::BinaryMessenger* messenger,
                      const std::string& app_name)
      : flutter_api_(messenger), app_name_(app_name) {}

  void OnIdTokenChanged(firebase::auth::Auth* auth) override {
    auto user = auth->current_user();

    if (user.is_valid()) {
      auto pigeon = FirebaseAuthWebosPlugin::UserToPigeon(user);

      flutter_api_.OnIdTokenChanged(
          app_name_,
          &pigeon,
          []() {},
          [](const FlutterError& error) {});

      flutter_api_.OnUserChanged(
          app_name_,
          &pigeon,
          []() {},
          [](const FlutterError& error) {});
    } else {
      flutter_api_.OnIdTokenChanged(
          app_name_,
          nullptr,
          []() {},
          [](const FlutterError& error) {});

      flutter_api_.OnUserChanged(
          app_name_,
          nullptr,
          []() {},
          [](const FlutterError& error) {});
    }
  }

 private:
  FirebaseAuthFlutterApi flutter_api_;
  std::string app_name_;
};

void FirebaseAuthWebosPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrar* registrar) {
  auto plugin = std::make_unique<FirebaseAuthWebosPlugin>();
  plugin->messenger_ = registrar->messenger();
  FirebaseAuthHostApi::SetUp(registrar->messenger(), static_cast<FirebaseAuthHostApi*>(plugin.get()));
  registrar->AddPlugin(std::move(plugin));
}

FirebaseAuthWebosPlugin::FirebaseAuthWebosPlugin() {}
FirebaseAuthWebosPlugin::~FirebaseAuthWebosPlugin() {

  for (auto& pair : auth_listeners_) {
    auto* auth = firebase::auth::Auth::GetAuth(
        FirebaseAppHolder::GetApp(pair.first));
    if (auth) {
      auth->RemoveAuthStateListener(pair.second.get());
    }
  }

  for (auto& pair : token_listeners_) {
    auto* auth = firebase::auth::Auth::GetAuth(
        FirebaseAppHolder::GetApp(pair.first));
    if (auth) {
      auth->RemoveIdTokenListener(pair.second.get());
    }
  }
}
firebase::auth::Auth* FirebaseAuthWebosPlugin::GetAuth(
    const std::string& app_name) {

  firebase::App* app = FirebaseAppHolder::GetApp(app_name);
  if (!app) return nullptr;

  auto* auth = firebase::auth::Auth::GetAuth(app);
  if (!auth) return nullptr;

  if (auth_listeners_.find(app_name) == auth_listeners_.end()) {

    auto auth_listener =
        std::make_unique<AuthStateListenerImpl>(messenger_, app_name);

    auth->AddAuthStateListener(auth_listener.get());

    auth_listeners_[app_name] = std::move(auth_listener);
  }

  if (token_listeners_.find(app_name) == token_listeners_.end()) {

    auto token_listener =
        std::make_unique<IdTokenListenerImpl>(messenger_, app_name);

    auth->AddIdTokenListener(token_listener.get());

    token_listeners_[app_name] = std::move(token_listener);
  }

  return auth;
}

PigeonUserDetails FirebaseAuthWebosPlugin::UserToPigeon(
    const firebase::auth::User& user) {
  PigeonUserInfo info(
    user.uid(),
    user.provider_id(),
    user.is_email_verified(),
    user.is_anonymous()
  );
  
  const std::string email = user.email();
  if (!email.empty()) info.set_email(email);
  const std::string display_name = user.display_name();
  if (!display_name.empty()) info.set_display_name(display_name);
  const std::string photo_url = user.photo_url();
  if (!photo_url.empty()) info.set_photo_url(photo_url);
  const std::string phone_number = user.phone_number();
  if (!phone_number.empty()) info.set_phone_number(phone_number);

  flutter::EncodableList provider_data;
  PigeonUserDetails details(info, provider_data);
  return details;
}

template <typename T>
void FirebaseAuthWebosPlugin::HandleAuthFuture(firebase::Future<T> future, std::function<void(ErrorOr<PigeonUserDetails>)> result) {
  future.OnCompletion([result](const firebase::Future<T>& completed_future) {
    if (completed_future.error() == firebase::auth::kAuthErrorNone) {
      const firebase::auth::User* user = nullptr;
      if constexpr (std::is_same_v<T, firebase::auth::AuthResult>) {
        user = &completed_future.result()->user;
      } else if constexpr (std::is_same_v<T, firebase::auth::User>) {
        user = completed_future.result();
      }
      
      if (user && user->is_valid()) {
        result(UserToPigeon(*user));
      } else {
        result(FlutterError("invalid_user", "Resulting user is invalid"));
      }
    } else {
      result(FlutterError(
          AuthErrorString(static_cast<firebase::auth::AuthError>(completed_future.error())),
          completed_future.error_message()));
    }
  });
}

void FirebaseAuthWebosPlugin::HandleVoidFuture(firebase::Future<void> future, std::function<void(std::optional<FlutterError>)> result) {
  future.OnCompletion([result](const firebase::Future<void>& completed_future) {
    if (completed_future.error() == firebase::auth::kAuthErrorNone) {
      result(std::nullopt);
    } else {
      result(FlutterError(
          AuthErrorString(static_cast<firebase::auth::AuthError>(completed_future.error())),
          completed_future.error_message()));
    }
  });
}

std::string FirebaseAuthWebosPlugin::AuthErrorString(firebase::auth::AuthError err) {
  switch (err) {
    // Success
    case firebase::auth::kAuthErrorNone:  // 0
      return "none";
    
    // Common errors
    case firebase::auth::kAuthErrorFailure:  // 1
      return "failure";
    case firebase::auth::kAuthErrorUnimplemented:  // -1
      return "unimplemented";
    
    // Custom token errors
    case firebase::auth::kAuthErrorInvalidCustomToken:  // 2
      return "invalid-custom-token";
    case firebase::auth::kAuthErrorCustomTokenMismatch:  // 3
      return "custom-token-mismatch";
    
    // Credential errors
    case firebase::auth::kAuthErrorInvalidCredential:  // 4
      return "invalid-credential";
    case firebase::auth::kAuthErrorUserDisabled:  // 5
      return "user-disabled";
    case firebase::auth::kAuthErrorAccountExistsWithDifferentCredentials:  // 6
      return "account-exists-with-different-credentials";
    case firebase::auth::kAuthErrorCredentialAlreadyInUse:  // 7
      return "credential-already-in-use";
    
    // Account info errors
    case firebase::auth::kAuthErrorOperationNotAllowed:  // 8
      return "operation-not-allowed";
    case firebase::auth::kAuthErrorEmailAlreadyInUse:  // 9
      return "email-already-in-use";
    case firebase::auth::kAuthErrorRequiresRecentLogin:  // 10
      return "requires-recent-login";
    
    // Email/password errors
    case firebase::auth::kAuthErrorInvalidEmail:  // 11
      return "invalid-email";
    case firebase::auth::kAuthErrorWrongPassword:  // 12
      return "wrong-password";
    case firebase::auth::kAuthErrorWeakPassword:  // 13
      return "weak-password";
    
    // Common API errors
    case firebase::auth::kAuthErrorTooManyRequests:  // 14
      return "too-many-requests";
    case firebase::auth::kAuthErrorUserNotFound:  // 15
      return "user-not-found";
    case firebase::auth::kAuthErrorNetworkRequestFailed:  // 16
      return "network-request-failed";
    case firebase::auth::kAuthErrorInvalidApiKey:  // 17
      return "invalid-api-key";
    case firebase::auth::kAuthErrorAppNotAuthorized:  // 18
      return "app-not-authorized";
    
    // Provider errors
    case firebase::auth::kAuthErrorProviderAlreadyLinked:  // 19
      return "provider-already-linked";
    case firebase::auth::kAuthErrorNoSuchProvider:  // 20
      return "no-such-provider";
    
    // Token errors
    case firebase::auth::kAuthErrorInvalidUserToken:  // 21
      return "invalid-user-token";
    case firebase::auth::kAuthErrorUserTokenExpired:  // 22
      return "user-token-expired";
    
    // User errors
    case firebase::auth::kAuthErrorUserMismatch:  // 23
      return "user-mismatch";
    
    // Action code errors
    case firebase::auth::kAuthErrorExpiredActionCode:  // 24
      return "expired-action-code";
    case firebase::auth::kAuthErrorInvalidActionCode:  // 25
      return "invalid-action-code";
    
    // Message/email errors
    case firebase::auth::kAuthErrorInvalidMessagePayload:  // 26
      return "invalid-message-payload";
    case firebase::auth::kAuthErrorInvalidSender:  // 27
      return "invalid-sender";
    case firebase::auth::kAuthErrorInvalidRecipientEmail:  // 28
      return "invalid-recipient-email";
    
    // Phone auth errors
    case firebase::auth::kAuthErrorInvalidPhoneNumber:  // 29
      return "invalid-phone-number";
    case firebase::auth::kAuthErrorMissingVerificationCode:  // 30
      return "missing-verification-code";
    case firebase::auth::kAuthErrorInvalidVerificationCode:  // 31
      return "invalid-verification-code";
    case firebase::auth::kAuthErrorMissingVerificationId:  // 32
      return "missing-verification-id";
    case firebase::auth::kAuthErrorInvalidVerificationId:  // 33
      return "invalid-verification-id";
    case firebase::auth::kAuthErrorSessionExpired:  // 34
      return "session-expired";
    
    // Missing data errors
    case firebase::auth::kAuthErrorMissingEmail:  // 35
      return "missing-email";
    case firebase::auth::kAuthErrorMissingContinueUri:  // 36
      return "missing-continue-uri";
    
    // Quota errors
    case firebase::auth::kAuthErrorQuotaExceeded:  // 37
      return "quota-exceeded";
    
    // Tenant errors
    case firebase::auth::kAuthErrorTenantIdMismatch:  // 38
      return "tenant-id-mismatch";
    case firebase::auth::kAuthErrorUnsupportedTenantOperation:  // 39
      return "unsupported-tenant-operation";
    
    // Client identifier errors
    case firebase::auth::kAuthErrorMissingClientIdentifier:  // 40
      return "missing-client-identifier";
    
    // Cancellation
    case firebase::auth::kAuthErrorCancelled:  // 41
      return "cancelled";

    // Legacy errors (for backward compatibility)
    case firebase::auth::kAuthErrorCaptchaCheckFailed:
      return "captcha-check-failed";
    case firebase::auth::kAuthErrorKeychainError:
      return "keychain-error";
    case firebase::auth::kAuthErrorWebInternalError:
      return "internal-error";
    case firebase::auth::kAuthErrorWebStorateUnsupported:
      return "web-storage-unsupported";
    case firebase::auth::kAuthErrorMissingPhoneNumber:
      return "missing-phone-number";

    default:
      return "unknown";
  }
}

void FirebaseAuthWebosPlugin::SignInWithEmailAndPassword(const std::string& app_name, const std::string& email, const std::string& password, std::function<void(ErrorOr<PigeonUserDetails>)> result) {
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }
  HandleAuthFuture(auth->SignInWithEmailAndPassword(email.c_str(), password.c_str()), result);
}

void FirebaseAuthWebosPlugin::CreateUserWithEmailAndPassword(const std::string& app_name, const std::string& email, const std::string& password, std::function<void(ErrorOr<PigeonUserDetails>)> result) {
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }
  HandleAuthFuture(auth->CreateUserWithEmailAndPassword(email.c_str(), password.c_str()), result);
}

void FirebaseAuthWebosPlugin::SignInAnonymously(const std::string& app_name, std::function<void(ErrorOr<PigeonUserDetails>)> result) {
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }
  HandleAuthFuture(auth->SignInAnonymously(), result);
}

void FirebaseAuthWebosPlugin::SignOut(const std::string& app_name, std::function<void(std::optional<FlutterError>)> result) {
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }
  auth->SignOut();
  result(std::nullopt);
}

void FirebaseAuthWebosPlugin::SignInWithCustomToken(const std::string& app_name,const std::string& token,std::function<void(ErrorOr<PigeonUserDetails>)> result) {
  auto* auth = GetAuth(app_name);
  if (!auth) {
    result(FlutterError("NO_APP", "App not initialized"));
    return;
  }
  HandleAuthFuture(
    auth->SignInWithCustomToken(token.c_str()),
    result
  );
}

void FirebaseAuthWebosPlugin::GetIdToken(const std::string& app_name,bool force_refresh,std::function<void(ErrorOr<std::string>)> result) {

  auto* auth = GetAuth(app_name);
  if (!auth) {
    result(FlutterError("NO_APP", "App not initialized"));
    return;
  }

  auto user = auth->current_user();
  if (!user.is_valid()) {
    result(FlutterError("NO_USER", "No current user"));
    return;
  }

  auto future = user.GetToken(force_refresh);

  future.OnCompletion([result](const firebase::Future<std::string>& f) {
    if (f.error() == firebase::auth::kAuthErrorNone) {
      result(*f.result());
    } else {
      result(FlutterError("token_error", f.error_message()));
    }
  });
}

void FirebaseAuthWebosPlugin::UpdateUserProfile(const std::string& app_name,
    const std::string* display_name,
    const std::string* photo_url,
    std::function<void(std::optional<FlutterError>)> result) {
  
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }

  auto user = auth->current_user();
  if (!user.is_valid()) {
    result(FlutterError("NO_USER", "No current user"));
    return;
  }

  firebase::auth::User::UserProfile profile;
  profile.display_name = display_name ? display_name->c_str() : nullptr;
  profile.photo_url = photo_url ? photo_url->c_str() : nullptr;

  auto future = user.UpdateUserProfile(profile);
  HandleVoidFuture(future, result);
}

void FirebaseAuthWebosPlugin::SendEmailVerification(const std::string& app_name,
    std::function<void(std::optional<FlutterError>)> result) {
  
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }

  auto user = auth->current_user();
  if (!user.is_valid()) {
    result(FlutterError("NO_USER", "No current user"));
    return;
  }

  auto future = user.SendEmailVerification();
  HandleVoidFuture(future, result);
}

void FirebaseAuthWebosPlugin::SendPasswordResetEmail(const std::string& app_name,
    const std::string& email,
    std::function<void(std::optional<FlutterError>)> result) {
  
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }

  auto future = auth->SendPasswordResetEmail(email.c_str());
  HandleVoidFuture(future, result);
}

void FirebaseAuthWebosPlugin::UpdatePassword(const std::string& app_name,
    const std::string& new_password,
    std::function<void(std::optional<FlutterError>)> result) {
  
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }

  auto user = auth->current_user();
  if (!user.is_valid()) {
    result(FlutterError("NO_USER", "No current user"));
    return;
  }

  auto future = user.UpdatePassword(new_password.c_str());
  HandleVoidFuture(future, result);
}

void FirebaseAuthWebosPlugin::Reload(const std::string& app_name,
    std::function<void(ErrorOr<PigeonUserDetails>)> result) {
  
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }

  auto user = auth->current_user();
  if (!user.is_valid()) {
    result(FlutterError("NO_USER", "No current user"));
    return;
  }

  auto future = user.Reload();
  future.OnCompletion([auth, result](const firebase::Future<void>& completed_future) {
    if (completed_future.error() == firebase::auth::kAuthErrorNone) {
      auto updated_user = auth->current_user();
      if (updated_user.is_valid()) {
        result(UserToPigeon(updated_user));
      } else {
        result(FlutterError("reload_failed", "User became invalid after reload"));
      }
    } else {
      result(FlutterError(
          AuthErrorString(static_cast<firebase::auth::AuthError>(completed_future.error())),
          completed_future.error_message()));
    }
  });
}

void FirebaseAuthWebosPlugin::Delete(const std::string& app_name,
    std::function<void(std::optional<FlutterError>)> result) {
  
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }

  auto user = auth->current_user();
  if (!user.is_valid()) {
    result(FlutterError("NO_USER", "No current user"));
    return;
  }

  auto future = user.Delete();
  HandleVoidFuture(future, result);
}

void FirebaseAuthWebosPlugin::UseAuthEmulator(const std::string& app_name,
    const std::string& host,
    int64_t port,
    std::function<void(std::optional<FlutterError>)> result) {
  
  auto* auth = GetAuth(app_name);
  if (!auth) { result(FlutterError("NO_APP", "App not initialized")); return; }

  auth->UseEmulator(host, static_cast<uint32_t>(port));
  result(std::nullopt);
}

}  // namespace firebase_auth_webos

void FirebaseAuthWebosPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  firebase_auth_webos::FirebaseAuthWebosPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
