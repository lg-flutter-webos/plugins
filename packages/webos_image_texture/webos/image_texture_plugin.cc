// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: LicenseRef-LGE-Proprietary

#include "include/webos_image_texture/image_texture_plugin.h"

#include <flutter/basic_message_channel.h>
#include <flutter/binary_messenger.h>
#include <flutter/encodable_value.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>
#include <flutter_webos.h>

#include <cstdlib>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <pbnjson.hpp>
#include <sstream>
#include <string>
#include <typeinfo>
#include <unistd.h>

#include "image_cache.h"
#include "image_texture.h"
#include "log.h"

namespace {
constexpr char VERSION[] = "0.2.12";
constexpr char TAG[] = "plugin";
constexpr char kSendVisibilityStatusMethodName[] = "visibilityStatus";
constexpr char kChannelName[] = "webos/image_texture";

constexpr char kInitializeMethod[] = "initialize";
constexpr char kClearCacheMethod[] = "clearCache";
constexpr char kCreateMethod[] = "create";
constexpr char kDisposeMethod[] = "dispose";
constexpr char kActivateMethod[] = "activate";
constexpr char kDeactivateMethod[] = "deactivate";
constexpr char kGetSizeMethod[] = "getSize";
constexpr char kPlayMethod[] = "play";
constexpr char kStopMethod[] = "stop";
constexpr char kLogMethod[] = "log";

///////////////////////////////////////////////////////
////          Class ImageTexturePlugin             ////
///////////////////////////////////////////////////////
class ImageTexturePlugin : public flutter::Plugin {
public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar *registrar,
                                    FlutterDesktopViewRef view);

  ImageTexturePlugin(flutter::PluginRegistrar *plugin_registrar,
                     flutter::TextureRegistrar *texture_registrar,
                     FlutterDesktopViewRef view);

  virtual ~ImageTexturePlugin();

private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void disposeAll();
  void handleVisibility(const std::string &response);

  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
  std::unordered_map<int64_t, std::unique_ptr<ImageTexture>> textures_;
  std::unordered_map<int64_t, std::unique_ptr<ImageTexture>> disposed_textures_;

  bool initialized_;
  bool visibility_;
  flutter::PluginRegistrar *plugin_registrar_;
  flutter::TextureRegistrar *texture_registrar_;
};

///////////////////////////////////////////////////////
////                Initialize                     ////
///////////////////////////////////////////////////////
void ImageTexturePlugin::RegisterWithRegistrar(
    flutter::PluginRegistrar *registrar, FlutterDesktopViewRef view) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), kChannelName,
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<ImageTexturePlugin>(
      registrar, registrar->texture_registrar(), view);

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });
  plugin->channel_ = std::move(channel);

  registrar->AddPlugin(std::move(plugin));
}

ImageTexturePlugin::ImageTexturePlugin(
    flutter::PluginRegistrar *plugin_registrar,
    flutter::TextureRegistrar *texture_registrar, FlutterDesktopViewRef view)
    : initialized_(false), visibility_(false),
      plugin_registrar_(plugin_registrar),
      texture_registrar_(texture_registrar) {

  curl_global_init(CURL_GLOBAL_ALL);

  ImageTextureThread::GetInstance().Start();

  FlutterDesktopViewAddListener(view, kSendVisibilityStatusMethodName,
                                std::bind(&ImageTexturePlugin::handleVisibility,
                                          this, std::placeholders::_1));
}

ImageTexturePlugin::~ImageTexturePlugin() {
  textures_.clear();
  disposed_textures_.clear();
  if (initialized_) {
    ImageCache::close();
  }
  ImageTextureThread::GetInstance().Stop();
  Texture::release_all();
  curl_global_cleanup();
}

void ImageTexturePlugin::disposeAll() {
  for (auto it = textures_.begin(); it != textures_.end(); ++it) {
    int64_t textureId = it->first;
    ImageTexture *ptexture = it->second.get();
    disposed_textures_[textureId] = std::move(it->second);
    ptexture->Dispose([this](int64_t textureId) -> void {
      disposed_textures_.erase(textureId);
    });
  }
  textures_.clear();
}

///////////////////////////////////////////////////////
////                HandleMethodCall               ////
///////////////////////////////////////////////////////
template <typename T>
bool GetValueFromEncodableMap(const flutter::EncodableMap *map, const char *key,
                              T &out) {
  if (map == nullptr) {
    return false;
  }
  auto iter = map->find(flutter::EncodableValue(key));
  if (iter != map->end() && !iter->second.IsNull()) {
    if (auto *value = std::get_if<T>(&iter->second)) {
      out = *value;
      return true;
    }
  }
  return false;
}

template <typename T>
T GetRequiredArg(const flutter::EncodableMap *arguments, const char *key) {
  T value;
  if (GetValueFromEncodableMap(arguments, key, value)) {
    return value;
  }
  std::string message = "[ImageTexturePlugin] No " + std::string(key) +
                        " provided or has invalid type or value.";
  throw std::invalid_argument(message);
}

static int64_t GetRequiredLongValue(const flutter::EncodableMap *arguments,
                                    const char *key) {
  auto iter = arguments->find(flutter::EncodableValue(key));
  if (iter != arguments->end() && !iter->second.IsNull()) {
    return iter->second.LongValue();
  }
  std::string message = "[ImageTexturePlugin] No " + std::string(key) +
                        " provided or has invalid type or value.";
  throw std::invalid_argument(message);
}

static std::string Replace(const std::string &url, const std::string &pattern,
                           const std::string &replace) {
  std::string result = url;
  std::string::size_type pos = 0;
  std::string::size_type offset = 0;

  while ((pos = result.find(pattern, offset)) != std::string::npos) {
    result.replace(result.begin() + pos, result.begin() + pos + pattern.size(),
                   replace);
    offset = pos + replace.size();
  }
  return result;
}

static std::string SafeUrl(const std::string &url) {
  std::string result;
  result = Replace(url, " ", "%20");
  result = Replace(result, "'", "%27");
  return result;
}

//////////////////////////////////////////////////////
//                Version Funcs                     //
//////////////////////////////////////////////////////
static std::string getVersionPath() {
  const char *appIdEnv = ::getenv("FLUTTER_APP_ID");
  const char *tempHomeEnv = ::getenv("FLUTTER_TEMP_HOME");
  if (!appIdEnv || !tempHomeEnv) {
    return std::string();
  }
  std::string appId = appIdEnv;
  // For HTTP
  std::string path = tempHomeEnv;
  path += "/";
  path += appId;
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  if (ec) {
    return std::string();
  }
  path += "/";
  path += "webos_image_texture.version";
  return path;
}

static std::string getVersion() {
  std::string version;
  std::string path = getVersionPath();
  if (path.empty()) {
    return version;
  }

  std::ifstream in(path);
  if (in.is_open()) {
    std::getline(in, version);
  }
  return version;
}

static void setVersion(const std::string &version) {
  std::string path = getVersionPath();
  if (path.empty()) {
    return;
  }

  std::ofstream out(path);
  if (out.is_open()) {
    out << version << std::endl;
  }
}

void ImageTexturePlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

  static const char *kInvalidArgument = "Invalid argument";

  try {
    if (!initialized_) {
      initialized_ = true;

      LogInfo(TAG) << "Version " << VERSION;
      if (getVersion() != VERSION) {
        setVersion(VERSION);
        ImageCache::clear();
      }
      ImageCache::open();
    }

    const std::string &method = method_call.method_name();
    const auto *arguments =
        std::get_if<flutter::EncodableMap>(method_call.arguments());

    if (method.compare(kInitializeMethod) == 0) {
      size_t bytes = GetRequiredLongValue(arguments, "maximumCacheBytes");
      ImageCache::set_maxbytes(bytes);
      disposeAll();
      result->Success();
    } else if (method.compare(kClearCacheMethod) == 0) {
      ImageCache::clear();
      result->Success();
    } else if (method.compare(kLogMethod) == 0) {
      std::string msg = GetRequiredArg<std::string>(arguments, "msg");
      LogInfo(TAG) << "Dart: " << msg;
    } else if (method.compare(kCreateMethod) == 0) {
      // Get TextureId
      auto texture = std::make_unique<ImageTexture>(
          plugin_registrar_, texture_registrar_, visibility_);
      int64_t textureId = texture->GetTextureId();
      if (textures_.find(textureId) != textures_.end()) {
        throw std::invalid_argument("duplicated texture");
      }

      // SetUrl
      std::string sourceType =
          GetRequiredArg<std::string>(arguments, "sourceType");
      int cacheWidth = GetRequiredArg<int32_t>(arguments, "cacheWidth");
      int cacheHeight = GetRequiredArg<int32_t>(arguments, "cacheHeight");
      if (sourceType == "DataSourceType.memory") {
        std::vector<uint8_t> bytes =
            GetRequiredArg<std::vector<uint8_t>>(arguments, "bytes");
        texture->SetDataSource(bytes, cacheWidth, cacheHeight);
      } else {
        std::string sourceUrl = GetRequiredArg<std::string>(arguments, "uri");
        std::string url = sourceUrl;
        if (sourceType == "DataSourceType.asset") {
          const char *assetsPath = ::getenv("FLUTTER_ASSETS_PATH");
          url = assetsPath ? assetsPath : "";
          if (!url.empty() && !sourceUrl.empty()) {
            if (url.back() != '/' && sourceUrl.front() != '/') {
              url += "/";
            }
          }
          url += sourceUrl;

          size_t size;
          std::string mtime;
          std::tie(size, mtime) = ImageCache::get_fileinfo(url);
          if (size == 0) {
            LogWarn(TAG) << sourceUrl
                         << " is not a asset image, fallback to file image";
            url = sourceUrl;
            sourceType = "DataSourceType.file";
          }
        } else if (sourceType == "DataSourceType.file") {
          std::string prefix = "file://";
          if (url.find(prefix) == 0) {
            url = url.substr(prefix.length());
          }
          if (url.front() != '/') {
            url.insert(0, "/");
          }
        } else if (sourceType == "DataSourceType.network") {
          url = SafeUrl(url);
        } else {
          throw std::invalid_argument("invalid sourceType");
        }
        texture->SetUrl(url, cacheWidth, cacheHeight);
      }
      textures_[textureId] = std::move(texture);
      result->Success(flutter::EncodableValue(textureId));
    } else if (method.compare(kDisposeMethod) == 0) {
      int64_t textureId = GetRequiredLongValue(arguments, "textureId");
      if (textures_.find(textureId) == textures_.end()) {
        throw std::invalid_argument("no texture");
      }
      ImageTexture *ptexture = textures_[textureId].get();
      disposed_textures_[textureId] = std::move(textures_[textureId]);
      textures_.erase(textureId);
      ptexture->Dispose([this](int64_t textureId) -> void {
        disposed_textures_.erase(textureId);
      });
      result->Success();
    } else if (method.compare(kActivateMethod) == 0) {
      int64_t textureId = GetRequiredLongValue(arguments, "textureId");
      if (textures_.find(textureId) == textures_.end()) {
        throw std::invalid_argument("no texture");
      }
      textures_[textureId]->Activate();
      result->Success();
    } else if (method.compare(kDeactivateMethod) == 0) {
      int64_t textureId = GetRequiredLongValue(arguments, "textureId");
      if (textures_.find(textureId) == textures_.end()) {
        throw std::invalid_argument("no texture");
      }
      textures_[textureId]->Deactivate();
      result->Success();
    } else if (method.compare(kGetSizeMethod) == 0) {
      int64_t textureId = GetRequiredLongValue(arguments, "textureId");
      if (textures_.find(textureId) == textures_.end()) {
        throw std::invalid_argument("no texture");
      }
      textures_[textureId]->GetSize(std::move(result));
    } else if (method.compare(kPlayMethod) == 0) {
      int64_t textureId = GetRequiredLongValue(arguments, "textureId");
      if (textures_.find(textureId) == textures_.end()) {
        throw std::invalid_argument("no texture");
      }
      textures_[textureId]->Play();
      result->Success();
    } else if (method.compare(kStopMethod) == 0) {
      int64_t textureId = GetRequiredLongValue(arguments, "textureId");
      if (textures_.find(textureId) == textures_.end()) {
        throw std::invalid_argument("no texture");
      }
      textures_[textureId]->Stop();
      result->Success();
    } else {
      result->NotImplemented();
    }
  } catch (const std::invalid_argument &error) {
    LogError(TAG) << "HandleMethodCall error " << error.what();
    result->Error(kInvalidArgument, error.what());
  } catch (const std::exception &e) {
    LogError(TAG) << "HandleMethodCall unexpected exception ("
                  << typeid(e).name() << "): " << e.what();
    result->Error("UnexpectedException", e.what());
  } catch (...) {
    LogError(TAG) << "HandleMethodCall unknown exception";
    result->Error("UnknownException", "unknown");
  }
}

void ImageTexturePlugin::handleVisibility(const std::string &response) {
  try {
    pbnjson::JValue doc = pbnjson::JDomParser::fromString(response);
    bool visibility = doc["visibility"].asBool();
    if (visibility_ == visibility) {
      return;
    }

    visibility_ = visibility;
    for (auto it = textures_.begin(); it != textures_.end(); ++it) {
      ImageTexture *ptexture = it->second.get();
      ptexture->Visibility(visibility);
    }
  } catch (const std::exception &e) {
    LogError(TAG) << "handleVisibility exception ("
                  << typeid(e).name() << "): " << e.what()
                  << ", response=[" << response << "]";
  } catch (...) {
    LogError(TAG) << "handleVisibility unknown exception, response=["
                  << response << "]";
  }
}

} // namespace

void ImageTexturePluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  ImageTexturePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar),
      FlutterDesktopPluginRegistrarGetView(registrar));
}