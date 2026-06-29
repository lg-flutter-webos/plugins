#include "firebase_functions_webos_plugin.h"
#include "firebase_core_webos/firebase_app_holder.h"
#include <firebase_functions_webos/firebase_functions_webos_plugin.h>

#include <flutter/plugin_registrar.h>
#include <firebase/functions.h>
#include <firebase/variant.h>
#include <string>
#include <map>
#include <vector>

namespace firebase_functions_webos {

using firebase::functions::Functions;
//using firebase::functions::HttpsCallable;
using firebase::functions::HttpsCallableResult;
using firebase::Variant;

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void FirebaseFunctionsWebosPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrar* registrar) {
  auto plugin = std::make_unique<FirebaseFunctionsWebosPlugin>();
  FirebaseFunctionsHostApi::SetUp(registrar->messenger(), plugin.get());
  registrar->AddPlugin(std::move(plugin));
}

FirebaseFunctionsWebosPlugin::FirebaseFunctionsWebosPlugin() {}
FirebaseFunctionsWebosPlugin::~FirebaseFunctionsWebosPlugin() {}

// ---------------------------------------------------------------------------
// GetFunctions helper
// ---------------------------------------------------------------------------

Functions* FirebaseFunctionsWebosPlugin::GetFunctions(
    const std::string& app_name, const std::string& region) {
  firebase::App* app = FirebaseAppHolder::GetApp(app_name);
  if (!app) return nullptr;
  return Functions::GetInstance(app, region.c_str());
}

// ---------------------------------------------------------------------------
// EncodableValue ↔ firebase::Variant converters
// ---------------------------------------------------------------------------

Variant FirebaseFunctionsWebosPlugin::EncodableToVariant(
    const flutter::EncodableValue& ev) {
  if (std::holds_alternative<std::monostate>(ev)) return Variant();
  if (std::holds_alternative<bool>(ev))
    return Variant(std::get<bool>(ev));
  if (std::holds_alternative<int32_t>(ev))
    return Variant(static_cast<int64_t>(std::get<int32_t>(ev)));
  if (std::holds_alternative<int64_t>(ev))
    return Variant(std::get<int64_t>(ev));
  if (std::holds_alternative<double>(ev))
    return Variant(std::get<double>(ev));
  if (std::holds_alternative<std::string>(ev))
    return Variant(std::get<std::string>(ev).c_str());

  if (std::holds_alternative<flutter::EncodableList>(ev)) {
    const auto& list = std::get<flutter::EncodableList>(ev);
    std::vector<Variant> v_list;
    v_list.reserve(list.size());
    for (const auto& item : list)
      v_list.push_back(EncodableToVariant(item));
    return Variant(v_list);
  }

  if (std::holds_alternative<flutter::EncodableMap>(ev)) {
    const auto& map = std::get<flutter::EncodableMap>(ev);
    std::map<Variant, Variant> v_map;
    for (const auto& [k, v] : map)
      v_map[EncodableToVariant(k)] = EncodableToVariant(v);
    return Variant(v_map);
  }

  // Uint8List and other binary types — not normally sent to Functions.
  return Variant();
}

flutter::EncodableValue FirebaseFunctionsWebosPlugin::VariantToEncodable(
    const Variant& v) {
  switch (v.type()) {
    case Variant::kTypeNull:
      return flutter::EncodableValue();
    case Variant::kTypeBool:
      return flutter::EncodableValue(v.bool_value());
    case Variant::kTypeInt64:
      return flutter::EncodableValue(v.int64_value());
    case Variant::kTypeDouble:
      return flutter::EncodableValue(v.double_value());
    case Variant::kTypeStaticString:
    case Variant::kTypeMutableString:
      return flutter::EncodableValue(std::string(v.string_value()));

    case Variant::kTypeVector: {
      flutter::EncodableList list;
      list.reserve(v.vector().size());
      for (const auto& item : v.vector())
        list.push_back(VariantToEncodable(item));
      return flutter::EncodableValue(list);
    }

    case Variant::kTypeMap: {
      flutter::EncodableMap map;
      for (const auto& [k, val] : v.map())
        map[VariantToEncodable(k)] = VariantToEncodable(val);
      return flutter::EncodableValue(map);
    }

    default:
      return flutter::EncodableValue();
  }
}

// ---------------------------------------------------------------------------
// CallFunction
// ---------------------------------------------------------------------------

void FirebaseFunctionsWebosPlugin::CallFunction(
    const HttpsCallableRequest& request,
    std::function<void(ErrorOr<HttpsCallableResponse>)> result) {

  const std::string app_name =
      request.app_name() ? *request.app_name() : "[DEFAULT]";

  Functions* functions = GetFunctions(app_name, request.region());
  if (!functions) {
    result(FlutterError("NO_APP", "Firebase app not initialised"));
    return;
  }

  firebase::functions::HttpsCallableReference ref =
      functions->GetHttpsCallable(request.function_name().c_str());

  if (!ref.is_valid()) {
    result(FlutterError("NO_FUNCTION", "Callable reference is null"));
    return;
  }

  Variant param_variant;
  if (request.parameters()) {
    param_variant = EncodableToVariant(*request.parameters());
  }

  // ⚠️ Correct call via internal reference
  firebase::Future<HttpsCallableResult> future =
      ref.Call(param_variant);

  future.OnCompletion(
      [result](const firebase::Future<HttpsCallableResult>& f) {
        if (f.error() == 0 && f.result() != nullptr) {

          HttpsCallableResponse response;
          response.set_data(
              VariantToEncodable(f.result()->data()));

          result(response);

        } else {
          result(FlutterError(
              std::to_string(f.error()),
              f.error_message() ? f.error_message() : "Unknown error"));
        }
      });
}

// ---------------------------------------------------------------------------
// UseFunctionsEmulator
// ---------------------------------------------------------------------------

void FirebaseFunctionsWebosPlugin::UseFunctionsEmulator(
    const std::string& app_name,
    const std::string& region,
    const std::string& host,
    int64_t port,
    std::function<void(std::optional<FlutterError>)> result) {
  Functions* functions = GetFunctions(app_name, region);
  if (!functions) {
    result(FlutterError("NO_APP", "Firebase app not initialised: " + app_name));
    return;
  }
  std::string origin = "http://" + host + ":" + std::to_string(port);
  functions->UseFunctionsEmulator(origin.c_str());
  result(std::nullopt);
}

}  // namespace firebase_functions_webos

void FirebaseFunctionsWebosPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  firebase_functions_webos::FirebaseFunctionsWebosPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
