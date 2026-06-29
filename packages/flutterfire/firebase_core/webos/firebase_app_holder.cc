#include "firebase_core_webos/firebase_app_holder.h"

#include <firebase/app.h>

std::map<std::string, firebase::App*> FirebaseAppHolder::apps_;
std::mutex FirebaseAppHolder::mutex_;

firebase::App* FirebaseAppHolder::GetOrCreate(const std::string& name,
                                              const firebase::AppOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = apps_.find(name);
    if (it != apps_.end()) {
        return it->second;
    }

    firebase::App* app = nullptr;
    if (name == "[DEFAULT]") {
        app = firebase::App::Create(options);
    } else {
        app = firebase::App::Create(options, name.c_str());
    }

    if (app == nullptr) {
        return nullptr;
    }

    apps_[name] = app;
    return app;
}

firebase::App* FirebaseAppHolder::GetApp(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = apps_.find(name);
    if (it != apps_.end()) {
        return it->second;
    }

    firebase::App* app = (name == "[DEFAULT]")
        ? firebase::App::GetInstance()
        : firebase::App::GetInstance(name.c_str());
    
    if (app) {
        apps_[name] = app;
    }
    
    return app;
}

bool FirebaseAppHolder::DeleteApp(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = apps_.find(name);
    if (it == apps_.end()) {
        return false;
    }

    delete it->second;
    apps_.erase(it);
    return true;
}
