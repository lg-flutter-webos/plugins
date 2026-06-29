#pragma once

#include <flutter/plugin_registrar.h>
#include <memory>
#include <map>
#include <string>
#include <vector>

#include "messages.g.h"
#include <firebase/app.h>
#include <firebase/remote_config.h>

namespace firebase_remote_config_webos {

class FirebaseRemoteConfigWebosPlugin : public flutter::Plugin,
                                        public FirebaseRemoteConfigHostApi {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar);

  FirebaseRemoteConfigWebosPlugin();
  virtual ~FirebaseRemoteConfigWebosPlugin();

  // Disallow copy and assign.
  FirebaseRemoteConfigWebosPlugin(const FirebaseRemoteConfigWebosPlugin&) = delete;
  FirebaseRemoteConfigWebosPlugin& operator=(const FirebaseRemoteConfigWebosPlugin&) = delete;

  // ---------------------------------------------------------------------------
  // FirebaseRemoteConfigHostApi overrides — signatures must match messages.g.h
  // ---------------------------------------------------------------------------

  void Initialize(
      const std::string& app_name,
      std::function<void(std::optional<FlutterError>)> result) override;

  void Fetch(
      const std::string& app_name,
      double cache_expiration_in_seconds,
      std::function<void(std::optional<FlutterError>)> result) override;

  void Activate(
      const std::string& app_name,
      std::function<void(ErrorOr<bool>)> result) override;

  void FetchAndActivate(
      const std::string& app_name,
      std::function<void(ErrorOr<bool>)> result) override;

  ErrorOr<std::string> GetString(
      const std::string& app_name,
      const std::string& key) override;

  ErrorOr<bool> GetBool(
      const std::string& app_name,
      const std::string& key) override;

  ErrorOr<int64_t> GetInt(
      const std::string& app_name,
      const std::string& key) override;

  ErrorOr<double> GetDouble(
      const std::string& app_name,
      const std::string& key) override;

  ErrorOr<PigeonRemoteConfigValue> GetValue(
      const std::string& app_name,
      const std::string& key) override;

  ErrorOr<flutter::EncodableMap> GetAll(const std::string& app_name) override;

  void SetDefaults(
      const std::string& app_name,
      const flutter::EncodableMap& default_parameters,
      std::function<void(std::optional<FlutterError>)> result) override;

  void SetInitialValues(
      const std::string& app_name,
      const flutter::EncodableMap& remote_config_values,
      std::function<void(std::optional<FlutterError>)> result) override;

  void SetConfigSettings(
      const std::string& app_name,
      const PigeonRemoteConfigSettings& settings,
      std::function<void(std::optional<FlutterError>)> result) override;

  ErrorOr<PigeonRemoteConfigSettings> GetConfigSettings(
      const std::string& app_name) override;

  void SetCustomSignals(
      const std::string& app_name,
      const flutter::EncodableMap& custom_signals,
      std::function<void(std::optional<FlutterError>)> result) override;

  void EnsureInitialized(
      const std::string& app_name,
      std::function<void(std::optional<FlutterError>)> result) override;

  ErrorOr<int64_t> GetLastFetchStatus(
    const std::string& app_name) override;

  ErrorOr<int64_t> GetLastFetchTime(
    const std::string& app_name) override;

  ErrorOr<PigeonRemoteConfigSettings> GetSettings(
      const std::string& app_name) override;

 private:
  // Returns the RemoteConfig singleton for the given app, or nullptr.
  firebase::remote_config::RemoteConfig* GetRemoteConfig(
      const std::string& app_name);

  // Map C++ ValueSource → Pigeon ValueSource enum.
  static PigeonValueSource CppSourceToPigeon(firebase::remote_config::ValueSource src);

  // Poll a Future<void> on a detached thread; deliver result on completion.
  void HandleVoidFuture(
      firebase::Future<void> future,
      std::function<void(std::optional<FlutterError>)> result);

  // Poll a Future<T> on a detached thread; deliver result on completion.
  template <typename T>
  void HandleFuture(
      firebase::Future<T> future,
      std::function<void(ErrorOr<T>)> result);
};

}  // namespace firebase_remote_config_webos
