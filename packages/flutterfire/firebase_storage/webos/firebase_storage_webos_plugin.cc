#include "firebase_storage_webos_plugin_impl.h"
#include "firebase_core_webos/firebase_app_holder.h"
#include <firebase_storage_webos/firebase_storage_webos_plugin.h>
#include <flutter/plugin_registrar.h>
#include <firebase/storage.h>
#include <string>
#include <map>

namespace firebase_storage_webos {

using namespace firebase::storage;

class StorageTaskListener : public Listener {
 public:
  StorageTaskListener(const std::string& handle,
                      FirebaseStorageWebosPlugin* plugin)
      : handle_(handle), plugin_(plugin) {}

  void OnProgress(Controller* controller) override {
    plugin_->NotifyTaskEvent(handle_,
                             controller->bytes_transferred(),
                             controller->total_byte_count(),
                             1 /* running */);
  }

  void OnPaused(Controller* controller) override {
    plugin_->NotifyTaskEvent(handle_,
                             controller->bytes_transferred(),
                             controller->total_byte_count(),
                             0 /* paused */);
  }

 private:
  std::string handle_;
  FirebaseStorageWebosPlugin* plugin_;
};


void FirebaseStorageWebosPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrar* registrar) {
  auto plugin =
      std::make_unique<FirebaseStorageWebosPlugin>(registrar->messenger());
  FirebaseStorageHostApi::SetUp(registrar->messenger(), plugin.get());
  registrar->AddPlugin(std::move(plugin));
}

FirebaseStorageWebosPlugin::FirebaseStorageWebosPlugin(
    flutter::BinaryMessenger* messenger)
    : messenger_(messenger) {
  flutter_api_ = std::make_unique<FirebaseStorageFlutterApi>(messenger_);
}

FirebaseStorageWebosPlugin::~FirebaseStorageWebosPlugin() {}


Storage* FirebaseStorageWebosPlugin::GetStorage(const std::string& app_name,
                                                  const std::string& bucket) {
  firebase::App* app = FirebaseAppHolder::GetApp(app_name);
  if (!app) return nullptr;
  if (!bucket.empty()) {
    std::string url = "gs://" + bucket;
    return Storage::GetInstance(app, url.c_str());
  }
  return Storage::GetInstance(app);
}

StorageReference FirebaseStorageWebosPlugin::GetRef(Storage* storage,
                                                     const std::string& path) {
  if (path.empty() || path == "/") return storage->GetReference();
  return storage->GetReference(path.c_str());
}

Metadata FirebaseStorageWebosPlugin::PigeonToNativeMetadata(
    const PigeonSettableMetaData& p) {
  Metadata m;

  if (p.cache_control())
    m.set_cache_control(p.cache_control()->c_str());

  if (p.content_disposition())
    m.set_content_disposition(p.content_disposition()->c_str());

  if (p.content_encoding())
    m.set_content_encoding(p.content_encoding()->c_str());

  if (p.content_language())
    m.set_content_language(p.content_language()->c_str());

  if (p.content_type())
    m.set_content_type(p.content_type()->c_str());

  if (p.custom_metadata()) {
    auto* meta_map = m.custom_metadata();
    if (meta_map) {
      for (const auto& [k, v] : *p.custom_metadata()) {
        if (std::holds_alternative<std::string>(k) &&
            std::holds_alternative<std::string>(v)) {
          (*meta_map)[std::get<std::string>(k)] =
              std::get<std::string>(v);
        }
      }
    }
  }

  return m;
}

PigeonFullMetaData FirebaseStorageWebosPlugin::NativeMetadataToPigeon(
    const Metadata& m) {
  flutter::EncodableMap meta_map;

  auto str = [&](const char* key, const char* val) {
    if (val && *val)
      meta_map[flutter::EncodableValue(key)] = flutter::EncodableValue(std::string(val));
  };
  auto i64 = [&](const char* key, int64_t val) {
    meta_map[flutter::EncodableValue(key)] = flutter::EncodableValue(val);
  };

  str("bucket",             m.bucket());
  str("name",               m.name());
  str("fullPath",           m.path());
  str("md5Hash",            m.md5_hash());
  str("cacheControl",       m.cache_control());
  str("contentDisposition", m.content_disposition());
  str("contentEncoding",    m.content_encoding());
  str("contentLanguage",    m.content_language());
  str("contentType",        m.content_type());
  i64("size",               static_cast<int64_t>(m.size_bytes()));
  i64("generation",         m.generation());
  i64("creationTimeMillis", m.creation_time());
  i64("updatedTimeMillis",  m.updated_time());

  const auto* raw_custom = m.custom_metadata();
  if (raw_custom && !raw_custom->empty()) {
    flutter::EncodableMap custom;
    for (const auto& [k, v] : *raw_custom)
      custom[flutter::EncodableValue(k)] = flutter::EncodableValue(v);
    meta_map[flutter::EncodableValue("customMetadata")] =
        flutter::EncodableValue(custom);
  }

  PigeonFullMetaData p;
  p.set_metadata(meta_map);
  return p;
}

void FirebaseStorageWebosPlugin::NotifyTaskEvent(
    const std::string& handle, int64_t bytes_transferred, int64_t total_bytes,
    int state_index, const Metadata* metadata) {
  
  PigeonTaskSnapshot snap;
  snap.set_handle(handle);
  snap.set_bytes_transferred(bytes_transferred);
  snap.set_total_bytes(total_bytes);
  snap.set_state(state_index);
  
  if (metadata) {
      snap.set_metadata(NativeMetadataToPigeon(*metadata));
  }

  flutter_api_->OnTaskEvent(handle, snap, []() {}, [](const FlutterError& error) {});
}

void FirebaseStorageWebosPlugin::NotifyTaskError(const std::string& handle,
                                                  int error_code,
                                                  const std::string& message) {
  flutter_api_->OnTaskError(handle, error_code, message, []() {}, [](const FlutterError&) {});
}


void FirebaseStorageWebosPlugin::SetMaxOperationRetryTime(
    const std::string& app_name, const std::string& bucket, int64_t time,
    std::function<void(std::optional<FlutterError>)> result) {
  if (auto* s = GetStorage(app_name, bucket))
    s->set_max_operation_retry_time(static_cast<double>(time) / 1000.0);
  result(std::nullopt);
}

void FirebaseStorageWebosPlugin::SetMaxUploadRetryTime(
    const std::string& app_name, const std::string& bucket, int64_t time,
    std::function<void(std::optional<FlutterError>)> result) {
  if (auto* s = GetStorage(app_name, bucket))
    s->set_max_upload_retry_time(static_cast<double>(time) / 1000.0);
  result(std::nullopt);
}

void FirebaseStorageWebosPlugin::SetMaxDownloadRetryTime(
    const std::string& app_name, const std::string& bucket, int64_t time,
    std::function<void(std::optional<FlutterError>)> result) {
  if (auto* s = GetStorage(app_name, bucket))
    s->set_max_download_retry_time(static_cast<double>(time) / 1000.0);
  result(std::nullopt);
}

void FirebaseStorageWebosPlugin::UseStorageEmulator(
    const std::string& app_name, const std::string& bucket,
    const std::string& host, int64_t port,
    std::function<void(std::optional<FlutterError>)> result) {
  if (auto* s = GetStorage(app_name, bucket))
    s->UseEmulator(host.c_str(), static_cast<int>(port));
  result(std::nullopt);
}

void FirebaseStorageWebosPlugin::ReferenceGetDownloadURL(
    const std::string& app_name, const std::string& bucket,
    const std::string& path,
    std::function<void(ErrorOr<std::string>)> result) {
  auto* s = GetStorage(app_name, bucket);
  if (!s) { result(FlutterError("NO_APP", "Storage not initialised")); return; }
  GetRef(s, path).GetDownloadUrl().OnCompletion(
      [result](const firebase::Future<std::string>& f) {
        if (f.error() == 0) result(*f.result());
        else result(FlutterError(std::to_string(f.error()), f.error_message()));
      });
}

void FirebaseStorageWebosPlugin::ReferenceGetMetaData(
    const std::string& app_name, const std::string& bucket,
    const std::string& path,
    std::function<void(ErrorOr<PigeonFullMetaData>)> result) {
  auto* s = GetStorage(app_name, bucket);
  if (!s) { result(FlutterError("NO_APP", "Storage not initialised")); return; }
  GetRef(s, path).GetMetadata().OnCompletion(
      [this, result](const firebase::Future<Metadata>& f) {
        if (f.error() == 0) result(NativeMetadataToPigeon(*f.result()));
        else result(FlutterError(std::to_string(f.error()), f.error_message()));
      });
}

void FirebaseStorageWebosPlugin::ReferenceUpdateMetaData(
    const std::string& app_name, const std::string& bucket,
    const std::string& path, const PigeonSettableMetaData& metadata,
    std::function<void(ErrorOr<PigeonFullMetaData>)> result) {
  auto* s = GetStorage(app_name, bucket);
  if (!s) { result(FlutterError("NO_APP", "Storage not initialised")); return; }
  Metadata native = PigeonToNativeMetadata(metadata);
  GetRef(s, path).UpdateMetadata(native).OnCompletion(
      [this, result](const firebase::Future<Metadata>& f) {
        if (f.error() == 0) result(NativeMetadataToPigeon(*f.result()));
        else result(FlutterError(std::to_string(f.error()), f.error_message()));
      });
}

void FirebaseStorageWebosPlugin::ReferenceDelete(
    const std::string& app_name, const std::string& bucket,
    const std::string& path,
    std::function<void(std::optional<FlutterError>)> result) {
  auto* s = GetStorage(app_name, bucket);
  if (!s) { result(FlutterError("NO_APP", "Storage not initialised")); return; }
  GetRef(s, path).Delete().OnCompletion(
      [result](const firebase::Future<void>& f) {
        if (f.error() == 0) result(std::nullopt);
        else result(FlutterError(std::to_string(f.error()), f.error_message()));
      });
}


void FirebaseStorageWebosPlugin::ReferenceGetData(
    const std::string& app_name, const std::string& bucket,
    const std::string& path, int64_t max_size,
    std::function<void(ErrorOr<std::vector<uint8_t>>)> result) {
  auto* s = GetStorage(app_name, bucket);
  if (!s) { result(FlutterError("NO_APP", "Storage not initialised")); return; }

  auto buffer = std::make_shared<std::vector<uint8_t>>(static_cast<size_t>(max_size));

  GetRef(s, path).GetBytes(buffer->data(), buffer->size()).OnCompletion(
      [result, buffer](const firebase::Future<size_t>& f) {
        if (f.error() == 0) {
          size_t bytes_read = *f.result();
          std::vector<uint8_t> data(buffer->begin(), buffer->begin() + bytes_read);
          result(data);
        } else {
          result(FlutterError(std::to_string(f.error()), f.error_message()));
        }
      });
}

void FirebaseStorageWebosPlugin::ReferenceList(
    const std::string&, const std::string&, const std::string&,
    const PigeonListOptions&,
    std::function<void(ErrorOr<PigeonListResult>)> result) {
  PigeonListResult r;
  r.set_items({});
  r.set_prefixs({});
  result(r);
}

void FirebaseStorageWebosPlugin::ReferenceListAll(
    const std::string&, const std::string&, const std::string&,
    std::function<void(ErrorOr<PigeonListResult>)> result) {
  PigeonListResult r;
  r.set_items({});
  r.set_prefixs({});
  result(r);
}

void FirebaseStorageWebosPlugin::TaskStartPutData(
    const std::string& app_name, const std::string& bucket,
    const std::string& path, const PigeonSettableMetaData* metadata,
    const std::vector<uint8_t>& data, const std::string& handle,
    std::function<void(std::optional<FlutterError>)> result) {
  auto* s = GetStorage(app_name, bucket);
  if (!s) { result(FlutterError("NO_APP", "Storage not initialised")); return; }

  TaskData td;
  td.buffer   = data;
  td.listener = std::make_unique<StorageTaskListener>(handle, this);

  firebase::Future<Metadata> future;
  auto ref = GetRef(s, path);
  if (metadata) {
    future = ref.PutBytes(td.buffer.data(), td.buffer.size(),
                          PigeonToNativeMetadata(*metadata),
                          td.listener.get(), &td.controller);
  } else {
    future = ref.PutBytes(td.buffer.data(), td.buffer.size(),
                          td.listener.get(), &td.controller);
  }

  {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    tasks_[handle] = std::move(td);
  }

  future.OnCompletion([this, handle](const firebase::Future<Metadata>& f) {
    if (f.error() == 0)
      NotifyTaskEvent(handle, f.result()->size_bytes(),
                      f.result()->size_bytes(), 2 /* success */, f.result());
    else if (f.error() == static_cast<int>(kErrorCancelled))
      NotifyTaskEvent(handle, 0, 0, 3 /* canceled */);
    else
      NotifyTaskError(handle, f.error(), f.error_message());
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    tasks_.erase(handle);
  });

  result(std::nullopt);
}

void FirebaseStorageWebosPlugin::TaskStartPutString(
    const std::string& app_name, const std::string& bucket,
    const std::string& path, const std::string& data,
    const PigeonStringFormat& /*format*/, const PigeonSettableMetaData* metadata,
    const std::string& handle,
    std::function<void(std::optional<FlutterError>)> result) {
  std::vector<uint8_t> bytes(data.begin(), data.end());
  TaskStartPutData(app_name, bucket, path, metadata, bytes, handle, result);
}

void FirebaseStorageWebosPlugin::TaskStartPutFile(
    const std::string& app_name, const std::string& bucket,
    const std::string& path, const std::string& file_path,
    const PigeonSettableMetaData* metadata, const std::string& handle,
    std::function<void(std::optional<FlutterError>)> result) {

  auto* s = GetStorage(app_name, bucket);

  if (!s) {
    result(FlutterError("NO_APP", "Storage not initialised"));
    return;
  }

  TaskData td;

  td.listener =
      std::make_unique<StorageTaskListener>(handle, this);

  auto ref = GetRef(s, path);
  firebase::Future<Metadata> future;

  if (metadata) {
    future = ref.PutFile(
            file_path.c_str(),
            PigeonToNativeMetadata(*metadata),
            td.listener.get(),
            &td.controller);
  } else {
    future = ref.PutFile(
            file_path.c_str(),
            td.listener.get(),
            &td.controller);
  }

  {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    tasks_[handle] = std::move(td);
  }

  future.OnCompletion(
      [this, handle](const firebase::Future<Metadata>& f) {
        if (f.error() == 0) {

          NotifyTaskEvent(
              handle,
              f.result()->size_bytes(),
              f.result()->size_bytes(),
              2,
              f.result());

        } else if (f.error() ==
                   static_cast<int>(kErrorCancelled)) {

          NotifyTaskEvent(
              handle,
              0,
              0,
              3);

        } else {

          NotifyTaskError(
              handle,
              f.error(),
              f.error_message());
        }

        std::lock_guard<std::mutex> lock(tasks_mutex_);
        tasks_.erase(handle);
      });

  result(std::nullopt);
}

void FirebaseStorageWebosPlugin::TaskStartWriteToFile(
    const std::string& app_name, const std::string& bucket,
    const std::string& path, const std::string& file_path,
    const std::string& handle,
    std::function<void(std::optional<FlutterError>)> result) {
  auto* s = GetStorage(app_name, bucket);
  if (!s) { result(FlutterError("NO_APP", "Storage not initialised")); return; }

  TaskData td;
  td.listener = std::make_unique<StorageTaskListener>(handle, this);

  firebase::Future<size_t> future =
      GetRef(s, path).GetFile(file_path.c_str(), td.listener.get(), &td.controller);

  {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    tasks_[handle] = std::move(td);
  }

  future.OnCompletion([this, handle](const firebase::Future<size_t>& f) {
    if (f.error() == 0) {
      int64_t sz = static_cast<int64_t>(*f.result());
      NotifyTaskEvent(handle, sz, sz, 2 /* success */);
    } else {
      NotifyTaskError(handle, f.error(), f.error_message());
    }
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    tasks_.erase(handle);
  });

  result(std::nullopt);
}


void FirebaseStorageWebosPlugin::TaskPause(
    const std::string& handle,
    std::function<void(std::optional<FlutterError>)> result) {
  std::lock_guard<std::mutex> lock(tasks_mutex_);
  auto it = tasks_.find(handle);
  if (it != tasks_.end()) it->second.controller.Pause();
  result(std::nullopt);
}

void FirebaseStorageWebosPlugin::TaskResume(
    const std::string& handle,
    std::function<void(std::optional<FlutterError>)> result) {
  std::lock_guard<std::mutex> lock(tasks_mutex_);
  auto it = tasks_.find(handle);
  if (it != tasks_.end()) it->second.controller.Resume();
  result(std::nullopt);
}

void FirebaseStorageWebosPlugin::TaskCancel(
    const std::string& handle,
    std::function<void(std::optional<FlutterError>)> result) {
  std::lock_guard<std::mutex> lock(tasks_mutex_);
  auto it = tasks_.find(handle);
  if (it != tasks_.end()) it->second.controller.Cancel();
  result(std::nullopt);
}

}  // namespace firebase_storage_webos

void FirebaseStorageWebosPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  firebase_storage_webos::FirebaseStorageWebosPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}

