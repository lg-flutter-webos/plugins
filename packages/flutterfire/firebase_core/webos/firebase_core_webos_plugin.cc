#include "firebase_core_webos/firebase_core_webos_plugin.h"
#include "firebase_core_webos_plugin_impl.h"
#include "firebase_core_webos/firebase_app_holder.h"
#include "messages.g.h"

#include <flutter/plugin_registrar.h>
#include <firebase/app.h>
#include <cstring>

namespace firebase_core_webos {

void FirebaseCoreWebosPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrar* registrar) {
  auto plugin = std::make_unique<FirebaseCoreWebosPlugin>();
  FirebaseCoreHostApi::SetUp(registrar->messenger(), plugin.get());
  registrar->AddPlugin(std::move(plugin));
}

FirebaseCoreWebosPlugin::FirebaseCoreWebosPlugin() {}
FirebaseCoreWebosPlugin::~FirebaseCoreWebosPlugin() {}

FirebaseAppPigeon FirebaseCoreWebosPlugin::AppToPigeon(firebase::App* app) {
    const firebase::AppOptions& opts = app->options();

    FirebaseOptionsPigeon pigeon_opts(
        opts.api_key(),
        opts.app_id(),
        opts.project_id()
    );

    if (opts.storage_bucket() && strlen(opts.storage_bucket()) > 0) {
        pigeon_opts.set_storage_bucket(opts.storage_bucket());
    }
    if (opts.database_url() && strlen(opts.database_url()) > 0) {
        pigeon_opts.set_database_url(opts.database_url());
    }
    if (opts.messaging_sender_id() && strlen(opts.messaging_sender_id()) >0) {
        pigeon_opts.set_messaging_sender_id(opts.messaging_sender_id());
    }

    FirebaseAppPigeon result(app->name(), pigeon_opts);
    return result;
}


void FirebaseCoreWebosPlugin::InitializeApp(
    const std::string& app_name,
    const FirebaseOptionsPigeon& options,
    std::function<void(ErrorOr<FirebaseAppPigeon>)> result) {

    if (app_name.empty()) {
        result(FlutterError("INVALID_ARGUMENT", "App name cannot be empty", flutter::EncodableValue()));
        return;
    }

    firebase::AppOptions app_options;
    app_options.set_api_key(options.api_key().c_str());
    app_options.set_app_id(options.app_id().c_str());
    app_options.set_project_id(options.project_id().c_str());

    if (options.storage_bucket() && !options.storage_bucket()->empty()) {
        app_options.set_storage_bucket(options.storage_bucket()->c_str());
    }
    if (options.database_url() && !options.database_url()->empty()) {
        app_options.set_database_url(options.database_url()->c_str());
    }
    if (options.messaging_sender_id() && !options.messaging_sender_id()->empty()) {
        app_options.set_messaging_sender_id(options.messaging_sender_id()->c_str());
    }

    firebase::App* app = FirebaseAppHolder::GetOrCreate(app_name, app_options);

    if (app == nullptr) {
        result(FlutterError("INIT_FAILED",
                            "firebase::App::Create returned null for app: " + app_name,
                            flutter::EncodableValue()));
        return;
    }

    result(AppToPigeon(app));
}


void FirebaseCoreWebosPlugin::GetApp(
    const std::string& app_name,
    std::function<void(ErrorOr<FirebaseAppPigeon>)> result) {

    firebase::App* app = FirebaseAppHolder::GetApp(app_name);

    if (app == nullptr) {
        result(FlutterError("APP_NOT_FOUND",
                            "No Firebase app initialized with name: " + app_name,
                            flutter::EncodableValue()));
        return;
    }

    result(AppToPigeon(app));
}


void FirebaseCoreWebosPlugin::DeleteApp(
    const std::string& app_name,
    std::function<void(std::optional<FlutterError>)> result) {

    bool deleted = FirebaseAppHolder::DeleteApp(app_name);

    if (!deleted) {
        result(FlutterError("APP_NOT_FOUND",
                            "No Firebase app found with name: " + app_name,
                            flutter::EncodableValue()));
        return;
    }

    result(std::nullopt);
}

}  // namespace firebase_core_webos

void FirebaseCoreWebosPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  firebase_core_webos::FirebaseCoreWebosPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
