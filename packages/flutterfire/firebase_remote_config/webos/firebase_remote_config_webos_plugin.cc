#include "firebase_remote_config_webos_plugin.h"
#include <firebase_remote_config_webos/firebase_remote_config_webos_plugin.h>
#include "firebase_core_webos/firebase_app_holder.h"

#include <thread>
#include <chrono>

namespace firebase_remote_config_webos {

using namespace firebase::remote_config;

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrar* registrar) {
  auto plugin = std::make_unique<FirebaseRemoteConfigWebosPlugin>();
  FirebaseRemoteConfigHostApi::SetUp(registrar->messenger(), plugin.get());
  registrar->AddPlugin(std::move(plugin));
}

FirebaseRemoteConfigWebosPlugin::FirebaseRemoteConfigWebosPlugin() {
}

FirebaseRemoteConfigWebosPlugin::~FirebaseRemoteConfigWebosPlugin() {
  // RemoteConfig instances are singletons managed by the C++ SDK — do NOT delete them.
}

// ---------------------------------------------------------------------------
// GetRemoteConfig helper
// ---------------------------------------------------------------------------

RemoteConfig* FirebaseRemoteConfigWebosPlugin::GetRemoteConfig(
    const std::string& app_name) {
  firebase::App* app = FirebaseAppHolder::GetApp(app_name);
  if (!app) {
    return nullptr;
  }
  RemoteConfig* rc = RemoteConfig::GetInstance(app);
  return rc;
}

// ---------------------------------------------------------------------------
// ValueSource converter
// ---------------------------------------------------------------------------

PigeonValueSource FirebaseRemoteConfigWebosPlugin::CppSourceToPigeon(
    firebase::remote_config::ValueSource src) {

  switch (src) {
    case kValueSourceRemoteValue:
      return PigeonValueSource::remoteValue;

    case kValueSourceDefaultValue:
      return PigeonValueSource::defaultValue;

    case kValueSourceStaticValue:
    default:
      return PigeonValueSource::staticValue;
  }
}

// ---------------------------------------------------------------------------
// Future helpers (OnCompletion — matches auth plugin pattern for this SDK)
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::HandleVoidFuture(
    firebase::Future<void> future,
    std::function<void(std::optional<FlutterError>)> result) {
  future.OnCompletion([result](const firebase::Future<void>& f) {
    if (f.error() == 0) {
      result(std::nullopt);
    } else {
      result(FlutterError(std::to_string(f.error()), f.error_message()));
    }
  });
}

template <typename T>
void FirebaseRemoteConfigWebosPlugin::HandleFuture(
    firebase::Future<T> future,
    std::function<void(ErrorOr<T>)> result) {
  future.OnCompletion([result](const firebase::Future<T>& f) {
    if (f.error() == 0) {
      result(*f.result());
    } else {
      result(FlutterError(std::to_string(f.error()), f.error_message()));
    }
  });
}

// ---------------------------------------------------------------------------
// Initialize — get the RemoteConfig singleton; if it exists we're done.
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::Initialize(
    const std::string& app_name,
    std::function<void(std::optional<FlutterError>)> result) {
  if (!GetRemoteConfig(app_name)) {
    result(FlutterError("NO_APP", "Firebase app not initialized: " + app_name));
    return;
  }
  result(std::nullopt);
}

// ---------------------------------------------------------------------------
// EnsureInitialized — wait for the internal initialization future.
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::EnsureInitialized(
    const std::string& app_name,
    std::function<void(std::optional<FlutterError>)> result) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    result(FlutterError("NO_APP", "Firebase app not initialized: " + app_name));
    return;
  }
  rc->EnsureInitialized().OnCompletion(
    [result](const firebase::Future<firebase::remote_config::ConfigInfo>& f) {
      if (f.error() == 0) {
        result(std::nullopt);
      } else {
        result(FlutterError(std::to_string(f.error()), f.error_message()));
      }
    });
}

// ---------------------------------------------------------------------------
// Fetch — must wait for the future; result on completion.
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::Fetch(
    const std::string& app_name,
    double cache_expiration_in_seconds,
    std::function<void(std::optional<FlutterError>)> result) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    result(FlutterError("NO_APP", "Firebase app not initialized: " + app_name));
    return;
  }

  firebase::Future<void> future;
  if (cache_expiration_in_seconds <= 0) {
    future = rc->Fetch();
  } else {
    // SDK takes uint64_t milliseconds.
    future = rc->Fetch(
        static_cast<uint64_t>(cache_expiration_in_seconds * 1000.0));
  }
  HandleVoidFuture(future, result);
}

// ---------------------------------------------------------------------------
// Activate
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::Activate(
    const std::string& app_name,
    std::function<void(ErrorOr<bool>)> result) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    result(FlutterError("NO_APP", "Firebase app not initialized: " + app_name));
    return;
  }
  HandleFuture(rc->Activate(), result);
}

// ---------------------------------------------------------------------------
// FetchAndActivate
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::FetchAndActivate(
    const std::string& app_name,
    std::function<void(ErrorOr<bool>)> result) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    result(FlutterError("NO_APP", "Firebase app not initialized: " + app_name));
    return;
  }
  HandleFuture(rc->FetchAndActivate(), result);
}

// ---------------------------------------------------------------------------
// Typed getters — all synchronous in the C++ SDK after activation.
// ---------------------------------------------------------------------------

ErrorOr<std::string> FirebaseRemoteConfigWebosPlugin::GetString(
    const std::string& app_name,
    const std::string& key) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    return FlutterError("NO_APP", "Firebase app not initialized: " + app_name);
  }
  std::string value = rc->GetString(key.c_str());
  return value;
}

ErrorOr<bool> FirebaseRemoteConfigWebosPlugin::GetBool(
    const std::string& app_name,
    const std::string& key) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    return FlutterError("NO_APP", "Firebase app not initialized: " + app_name);
  }
  bool value = rc->GetBoolean(key.c_str());
  return value;
}

ErrorOr<int64_t> FirebaseRemoteConfigWebosPlugin::GetInt(
    const std::string& app_name,
    const std::string& key) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    return FlutterError("NO_APP", "Firebase app not initialized: " + app_name);
  }
  int64_t value = static_cast<int64_t>(rc->GetLong(key.c_str()));
  return value;
}

ErrorOr<double> FirebaseRemoteConfigWebosPlugin::GetDouble(
    const std::string& app_name,
    const std::string& key) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    return FlutterError("NO_APP", "Firebase app not initialized: " + app_name);
  }
  double value = rc->GetDouble(key.c_str());
  return value;
}

ErrorOr<PigeonRemoteConfigValue> FirebaseRemoteConfigWebosPlugin::GetValue(
    const std::string& app_name,
    const std::string& key) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    return FlutterError("NO_APP", "Firebase app not initialized: " + app_name);
  }

  firebase::remote_config::ValueInfo info;
  std::string str_value = rc->GetString(key.c_str(), &info);

  PigeonRemoteConfigValue pv(CppSourceToPigeon(info.source));
  pv.set_value(str_value);
  return pv;
}

// ---------------------------------------------------------------------------
// GetAll — iterate all known keys and return map of key → PigeonRemoteConfigValue
// ---------------------------------------------------------------------------
ErrorOr<flutter::EncodableMap> FirebaseRemoteConfigWebosPlugin::GetAll(
    const std::string& app_name) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    return FlutterError(
        "NO_APP",
        "Firebase app not initialized: " + app_name);
  }

  flutter::EncodableMap map;
  std::vector<std::string> keys = rc->GetKeys();

  for (const auto& key : keys) {
    firebase::remote_config::ValueInfo info;
    std::string str_value = rc->GetString(key.c_str(), &info);

    PigeonRemoteConfigValue value(
        CppSourceToPigeon(info.source));
    value.set_value(str_value);

    map[flutter::EncodableValue(key)] =
        flutter::CustomEncodableValue(value);
  }

  return map;
}

// ---------------------------------------------------------------------------
// SetDefaults — keep strings alive via shared_ptr until future completes.
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::SetDefaults(
    const std::string& app_name,
    const flutter::EncodableMap& default_parameters,
    std::function<void(std::optional<FlutterError>)> result) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    result(FlutterError("NO_APP", "Firebase app not initialized: " + app_name));
    return;
  }


  // Keep the strings alive for the duration of the async call.
  struct ReqData {
    std::vector<std::string> keys;
    std::vector<std::string> vals;
    std::vector<ConfigKeyValue> kv_pairs;
  };
  auto req = std::make_shared<ReqData>();

  for (const auto& [k, v] : default_parameters) {
    if (!std::holds_alternative<std::string>(k)) continue;
    std::string val_str;
    if (std::holds_alternative<std::string>(v))
      val_str = std::get<std::string>(v);
    else if (std::holds_alternative<int32_t>(v))
      val_str = std::to_string(std::get<int32_t>(v));
    else if (std::holds_alternative<int64_t>(v))
      val_str = std::to_string(std::get<int64_t>(v));
    else if (std::holds_alternative<double>(v))
      val_str = std::to_string(std::get<double>(v));
    else if (std::holds_alternative<bool>(v))
      val_str = std::get<bool>(v) ? "true" : "false";

    req->keys.push_back(std::get<std::string>(k));
    req->vals.push_back(std::move(val_str));
  }

  req->kv_pairs.reserve(req->keys.size());
  for (size_t i = 0; i < req->keys.size(); ++i) {
    req->kv_pairs.push_back({req->keys[i].c_str(), req->vals[i].c_str()});
  }

  auto future = rc->SetDefaults(req->kv_pairs.data(), req->kv_pairs.size());
  future.OnCompletion([req, result](const firebase::Future<void>& f) {
    if (f.error() == 0) {
      result(std::nullopt);
    } else {
      result(FlutterError(std::to_string(f.error()), f.error_message()));
    }
  });
}

// ---------------------------------------------------------------------------
// SetInitialValues — not exposed in C++ SDK; no-op.
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::SetInitialValues(
    const std::string& /*app_name*/,
    const flutter::EncodableMap& /*remote_config_values*/,
    std::function<void(std::optional<FlutterError>)> result) {
  result(std::nullopt);
}

// ---------------------------------------------------------------------------
// SetConfigSettings
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::SetConfigSettings(
    const std::string& app_name,
    const PigeonRemoteConfigSettings& settings,
    std::function<void(std::optional<FlutterError>)> result) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    result(FlutterError("NO_APP", "Firebase app not initialized: " + app_name));
    return;
  }
  ConfigSettings cs;
  cs.fetch_timeout_in_milliseconds =
      static_cast<uint64_t>(settings.fetch_timeout() * 1000.0);
  cs.minimum_fetch_interval_in_milliseconds =
      static_cast<uint64_t>(settings.minimum_fetch_interval() * 1000.0);
  HandleVoidFuture(rc->SetConfigSettings(cs), result);
}

// ---------------------------------------------------------------------------
// GetConfigSettings / GetSettings (alias)
// ---------------------------------------------------------------------------

ErrorOr<PigeonRemoteConfigSettings> FirebaseRemoteConfigWebosPlugin::GetConfigSettings(
    const std::string& app_name) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    return FlutterError("NO_APP", "Firebase app not initialized: " + app_name);
  }

  ConfigSettings cs = rc->GetConfigSettings();
  return PigeonRemoteConfigSettings(
      static_cast<double>(cs.fetch_timeout_in_milliseconds) / 1000.0,
      static_cast<double>(cs.minimum_fetch_interval_in_milliseconds) / 1000.0);
}

ErrorOr<PigeonRemoteConfigSettings> FirebaseRemoteConfigWebosPlugin::GetSettings(
    const std::string& app_name) {
  return GetConfigSettings(app_name);
}

// ---------------------------------------------------------------------------
// SetCustomSignals — not in C++ SDK; no-op.
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPlugin::SetCustomSignals(
    const std::string& /*app_name*/,
    const flutter::EncodableMap& /*custom_signals*/,
    std::function<void(std::optional<FlutterError>)> result) {
  result(std::nullopt);
}

// ---------------------------------------------------------------------------
// GetLastFetchStatus
// ---------------------------------------------------------------------------

ErrorOr<int64_t> FirebaseRemoteConfigWebosPlugin::GetLastFetchStatus(
    const std::string& app_name) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    return FlutterError(
        "NO_APP",
        "Firebase app not initialized: " + app_name);
  }

  const ConfigInfo info = rc->GetInfo();
  return static_cast<int64_t>(info.last_fetch_status);
}

// ---------------------------------------------------------------------------
// GetLastFetchTime
// ---------------------------------------------------------------------------
ErrorOr<int64_t> FirebaseRemoteConfigWebosPlugin::GetLastFetchTime(
    const std::string& app_name) {
  auto* rc = GetRemoteConfig(app_name);
  if (!rc) {
    return FlutterError(
        "NO_APP",
        "Firebase app not initialized: " + app_name);
  }

  const ConfigInfo info = rc->GetInfo();

  return static_cast<int64_t>(info.fetch_time);
}

}  // namespace firebase_remote_config_webos

// ---------------------------------------------------------------------------
// C registration entry point (called by the Flutter embedder)
// ---------------------------------------------------------------------------

void FirebaseRemoteConfigWebosPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  firebase_remote_config_webos::FirebaseRemoteConfigWebosPlugin::
      RegisterWithRegistrar(
          flutter::PluginRegistrarManager::GetInstance()
              ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
