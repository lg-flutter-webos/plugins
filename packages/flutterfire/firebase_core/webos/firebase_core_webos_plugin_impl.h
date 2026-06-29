#ifndef FIREBASE_CORE_WEBOS_PLUGIN_IMPL_H_
#define FIREBASE_CORE_WEBOS_PLUGIN_IMPL_H_

#include <flutter/plugin_registrar.h>
#include <memory>
#include <string>
#include <vector>

#include "messages.g.h"
#include <firebase/app.h>

namespace firebase_core_webos {

class FirebaseCoreWebosPlugin : public flutter::Plugin,
                                public FirebaseCoreHostApi {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar);
  FirebaseCoreWebosPlugin();
  virtual ~FirebaseCoreWebosPlugin();

  FirebaseCoreWebosPlugin(const FirebaseCoreWebosPlugin&) = delete;
  FirebaseCoreWebosPlugin& operator=(const FirebaseCoreWebosPlugin&) = delete;

  void InitializeApp(
      const std::string& app_name,
      const FirebaseOptionsPigeon& options,
      std::function<void(ErrorOr<FirebaseAppPigeon>)> result) override;

  void GetApp(
      const std::string& app_name,
      std::function<void(ErrorOr<FirebaseAppPigeon>)> result) override;

  void DeleteApp(
      const std::string& app_name,
      std::function<void(std::optional<FlutterError>)> result) override;

 private:
  static FirebaseAppPigeon AppToPigeon(firebase::App* app);
};

}  // namespace firebase_core_webos

#endif  // FIREBASE_CORE_WEBOS_PLUGIN_IMPL_H_