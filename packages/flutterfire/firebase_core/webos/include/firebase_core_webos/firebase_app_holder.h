#pragma once

#include <firebase/app.h>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class FirebaseAppHolder {
 public:

  static firebase::App* GetOrCreate(const std::string& name,
                                    const firebase::AppOptions& options);
  static firebase::App* GetApp(const std::string& name = "[DEFAULT]");
  static bool DeleteApp(const std::string& name);

 private:
  static std::map<std::string, firebase::App*> apps_;
  static std::mutex mutex_;
};
