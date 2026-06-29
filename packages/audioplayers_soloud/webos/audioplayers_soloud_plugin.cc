// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "include/audioplayers_soloud/audioplayers_soloud_plugin.h"

#include <flutter/encodable_value.h>
#include <flutter/event_channel.h>
#include <flutter/event_sink.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>
#include <flutter_webos.h>

#include <map>
#include <memory>
#include <string>
#include <variant>
#include <mutex>

#include <pbnjson.hpp>
#include "audio_player.h"
#include "log.h"
#include "dummy_player.h"
#include "wave_generator.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "AudioplayersWebOsPlugin::"

static const size_t MAX_PIPELINES = 31;
static constexpr char kSendVisibilityStatusMethodName[] = "visibilityStatus";
static const char *kInvalidArgument = "Invalid argument";

namespace {

template <typename T>
bool GetValueFromEncodableMap(const flutter::EncodableMap *map, const char *key,
                              T &out) {
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
  std::string message =
      "No " + std::string(key) + " provided or has invalid type or value.";
  throw std::invalid_argument(message);
}

static ReleaseMode StringToReleaseMode(std::string release_mode) {
  if (release_mode == "ReleaseMode.release") {
    return ReleaseMode::kRelease;
  } else if (release_mode == "ReleaseMode.loop") {
    return ReleaseMode::kLoop;
  } else if (release_mode == "ReleaseMode.stop") {
    return ReleaseMode::kStop;
  }
  throw std::invalid_argument("Invalid release mode.");
}

class AudioplayersWebOsPlugin : public flutter::Plugin {
public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar,
                                    FlutterDesktopViewRef view);
  static void WaitResumed();

  AudioplayersWebOsPlugin(flutter::PluginRegistrar* plugin_registrar,
                          FlutterDesktopViewRef view);

  virtual ~AudioplayersWebOsPlugin();

private:
  void handleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void handleGlobalMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void SetUpGlobalEventChannel(flutter::BinaryMessenger *messenger);
  AudioPlayer *getAudioPlayer(const std::string &player_id);

  void lockUnlock();
  bool isLocked();
  void handleVisibility(const std::string& response);

  void disposeAllPlayers();
  void dispose(const std::string& player_id);
  void historyAdd(const std::string& player_id);
  void historyErase(const std::string& player_id);

  std::string headTrimmer(const std::string& path);

  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> global_channel_;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      global_event_channel_;
  std::map<std::string, std::unique_ptr<AudioPlayer>> players_;
  std::map<std::string, bool> forced_pauses_;
  std::map<std::string, int> debug_ids_;
  std::vector<std::string> create_history_;
  flutter::PluginRegistrar *registrar_;
  std::string app_id_;
  bool background_run_ = false;

  std::unique_ptr<WaveGenerator> wave_generator_;
  std::unique_ptr<DummyPlayer> dummy_player_;

  bool visibility_ = false;
  bool system_ui_ = false;
  static std::mutex platform_lock_;
};

std::mutex AudioplayersWebOsPlugin::platform_lock_;

void AudioplayersWebOsPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrar *registrar,
    FlutterDesktopViewRef view) {
  auto plugin = std::make_unique<AudioplayersWebOsPlugin>(registrar, view);
  {
    auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
        registrar->messenger(), "xyz.luan/audioplayers",
        &flutter::StandardMethodCodec::GetInstance());

    channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result) {
          plugin_pointer->handleMethodCall(call, std::move(result));
        });

    plugin->channel_ = std::move(channel);
  }
  {
    auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
        registrar->messenger(), "xyz.luan/audioplayers.global",
        &flutter::StandardMethodCodec::GetInstance());

    channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result) {
          plugin_pointer->handleGlobalMethodCall(call, std::move(result));
        });

    plugin->global_channel_ = std::move(channel);
    plugin->SetUpGlobalEventChannel(registrar->messenger());
  }
  registrar->AddPlugin(std::move(plugin));
}

void AudioplayersWebOsPlugin::SetUpGlobalEventChannel(flutter::BinaryMessenger *messenger) {
  std::string name = "xyz.luan/audioplayers.global/events";
  auto channel =
      std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
          messenger, name, &flutter::StandardMethodCodec::GetInstance());
  auto handler = std::make_unique<
      flutter::StreamHandlerFunctions<flutter::EncodableValue>>(
      [&](const flutter::EncodableValue *arguments,
          std::unique_ptr<flutter::EventSink<>> &&events)
          -> std::unique_ptr<flutter::StreamHandlerError<>> {
        return nullptr;
      },
      [&](const flutter::EncodableValue *arguments)
          -> std::unique_ptr<flutter::StreamHandlerError<>> {
        return nullptr;
      });
  channel->SetStreamHandler(std::move(handler));

  global_event_channel_ = std::move(channel);
}

void AudioplayersWebOsPlugin::WaitResumed() {
  platform_lock_.lock();
  platform_lock_.unlock();
}

AudioplayersWebOsPlugin::AudioplayersWebOsPlugin(
    flutter::PluginRegistrar *registrar,
    FlutterDesktopViewRef view)
  : registrar_(registrar) {
  app_id_ = ::getenv("FLUTTER_APP_ID");

  std::string path = ::getenv("FLUTTER_BUNDLE_PATH");
  path += "/appinfo.json";
  pbnjson::JValue doc = pbnjson::JDomParser::fromFile(path.c_str());

  if (app_id_.find("com.webos.") == 0) {
    background_run_ = doc["enableBackgroundRun"].isBoolean()
                    ? doc["enableBackgroundRun"].asBool()
                    : false;
  }
  std::string type = doc["defaultWindowType"].isString()
                   ? doc["defaultWindowType"].asString()
                   : "card";
  system_ui_ = (type != "card") ? true : false;
  if (system_ui_) {
    LogInfo(TAG) << "non-card type application !!!";
  } else {
    // Workaround fix BT sound issue.
    wave_generator_ = std::make_unique<WaveGenerator>();
    dummy_player_ = std::make_unique<DummyPlayer>();
  }

  FlutterDesktopViewAddListener(
      view,
      kSendVisibilityStatusMethodName,
      std::bind(&AudioplayersWebOsPlugin::handleVisibility, this, std::placeholders::_1));

  lockUnlock();
}

AudioplayersWebOsPlugin::~AudioplayersWebOsPlugin() {
  disposeAllPlayers();

  if (!system_ui_) {
    dummy_player_ = nullptr;
    wave_generator_ = nullptr;
  }
}

void AudioplayersWebOsPlugin::lockUnlock() {
  if (!background_run_) {
     if (visibility_) {
       platform_lock_.unlock();
     } else {
       platform_lock_.lock();
     }
  }
}

bool AudioplayersWebOsPlugin::isLocked() {
  return (!background_run_ && !visibility_) ? true : false;
}

void AudioplayersWebOsPlugin::handleVisibility(const std::string& response) {
  pbnjson::JValue doc = pbnjson::JDomParser::fromString(response);

  bool visibility = doc["visibility"].asBool();
  if (visibility_ == visibility) {
    return;
  }

  visibility_ = visibility;
  if (visibility) {
    if (dummy_player_) {
      dummy_player_->SetUrl(wave_generator_->GetUrl());
    }
  } else {
    if (dummy_player_) {
      dummy_player_->Release();
    }
    forced_pauses_.clear();
  }
  lockUnlock();

  for (std::string& playerId : create_history_) {
    AudioPlayer* player = getAudioPlayer(playerId);
    if (player == nullptr) {
      continue;
    }

    if (visibility) {
      if (forced_pauses_.find(playerId) != forced_pauses_.end()) {
        player->Play();
        forced_pauses_.erase(playerId);
      }
    } else if (!background_run_) {
      if (player->IsPlaying()) {
        player->Pause();
        forced_pauses_[playerId] = true;
      }
    }
  }
}

static std::string Replace(const std::string& url, const std::string& pattern, const std::string& replace) {
  std::string result = url;
  std::string::size_type pos = 0;
  std::string::size_type offset = 0;
  while ((pos = result.find(pattern, offset)) != std::string::npos) {
    result.replace(result.begin() + pos, result.begin() + pos + pattern.size(), replace);
    offset = pos + replace.size();
  }
  return result;
}

static std::string SafeUrl(const std::string& url) {
  std::string result;
  result = Replace(url, " ", "%20");
  result = Replace(result, "'", "%27");
  return result;
}

void AudioplayersWebOsPlugin::handleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const auto *arguments =
      std::get_if<flutter::EncodableMap>(method_call.arguments());
  if (!arguments) {
    result->Error(kInvalidArgument, "No arguments provided.");
    return;
  }

  // player_id
  std::string player_id;
  if (!GetValueFromEncodableMap(arguments, "playerId", player_id)) {
    result->Error(kInvalidArgument, "No playerId provided.");
    return;
  }

  const std::string &method_name = method_call.method_name();
  if (method_name == "create") {
    result->Success();

    static int uid = 0;
    int debug_id = uid++;
    auto audio_player = std::make_unique<AudioPlayer>(registrar_,
                                                player_id, debug_id);
    debug_ids_[player_id] = debug_id;
    players_[player_id] = std::move(audio_player);
    historyAdd(player_id);
    LogInfo(TAG) << "Create() " << players_.size() << " players are running";
    return;
  }
  if (method_name == "dispose") {
    result->Success();
    dispose(player_id);
    LogInfo(TAG) << "Dispose() " << players_.size() << " players are running";
    return;
  }

  // player
  AudioPlayer *player = getAudioPlayer(player_id);
  if (!player) {
    result->Error(kInvalidArgument,
                "No AudioPlayer" + player_id + " is exist.");
    return;
  }

  if (method_name == "setSourceBytes") {
    result->Success();
    std::vector<uint8_t> bytes =
        GetRequiredArg<std::vector<uint8_t>>(arguments, "bytes");

    std::string mime_type;
    GetValueFromEncodableMap(arguments, "mimeType", mime_type);
    player->SetDataSource(bytes, mime_type);
  } else if (method_name == "setSourceUrl") {
    result->Success();
    bool is_local = false;
    GetValueFromEncodableMap(arguments, "isLocal", is_local);

    std::string url = GetRequiredArg<std::string>(arguments, "url");
    if (is_local) {
      const std::string file_protocol_prefix = "file://";
      if (url.find("/") == 0) {
        url = file_protocol_prefix + url;
      } else if (url.find(file_protocol_prefix) != 0) {
        std::string assets_path = ::getenv("FLUTTER_ASSETS_PATH");
        url = file_protocol_prefix + assets_path + "/" + headTrimmer(url);
      }
    } else {
      url = SafeUrl(url);
    }
    std::string mime_type;
    GetValueFromEncodableMap(arguments, "mimeType", mime_type);
    player->SetUrl(url, mime_type);
  } else if (method_name == "resume") {
    result->Success();
    player->Play();
  } else if (method_name == "pause") {
    result->Success();
    if (!visibility_ && player->IsPlaying()) {
      forced_pauses_[player_id] = true;
      LogDebug(TAG) << "forced_pause(" << debug_ids_[player_id] << ") by audioplayers_plugin";
    }
    player->Pause();
  } else if (method_name == "stop") {
    result->Success();
    player->Stop();
  } else if (method_name == "release") {
    result->Success();
    player->Release();
  } else if (method_name == "seek") {
    result->Success();
    player->Seek(GetRequiredArg<int32_t>(arguments, "position"));
  } else if (method_name == "setVolume") {
    result->Success();
    double volume = GetRequiredArg<double>(arguments, "volume");
    player->SetVolume(volume);
  } else if (method_name == "setPlaybackRate") {
    result->Success();
    double rate = GetRequiredArg<double>(arguments, "playbackRate");
    player->SetPlaybackRate(rate);
  } else if (method_name == "setBalance") {
    result->Success();
    double balance = GetRequiredArg<double>(arguments, "balance");
    player->SetBalance(balance);
  } else if (method_name == "setReleaseMode") {
    result->Success();
    std::string release_mode =
        GetRequiredArg<std::string>(arguments, "releaseMode");
    player->SetReleaseMode(StringToReleaseMode(release_mode));
  } else if (method_name == "getDuration") {
    result->Success(flutter::EncodableValue(player->GetDuration()));
  } else if (method_name == "getCurrentPosition") {
    result->Success(flutter::EncodableValue(player->GetPosition()));
  } else if (method_name == "setPlayerMode") {
    result->Success();
    bool low_latency =
            GetRequiredArg<std::string>(arguments, "playerMode") ==
            "PlayerMode.lowLatency";
    player->SetLatencyMode(low_latency);
  } else {
    LogError(TAG) << method_name << " is not implemented !!!";
    result->NotImplemented();
  }
}

void AudioplayersWebOsPlugin::handleGlobalMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const std::string &method_name = method_call.method_name();
  if (method_name == "init") {
    disposeAllPlayers();
    result->Success();
  } else {
    result->NotImplemented();
  }
}

AudioPlayer* AudioplayersWebOsPlugin::getAudioPlayer(const std::string &player_id) {
  auto it = players_.find(player_id);
  if (it != players_.end()) {
    return it->second.get();
  }
  return nullptr;
}

void AudioplayersWebOsPlugin::disposeAllPlayers() {
  std::vector<std::string> ids = create_history_;
  for (std::string& id : ids) {
    dispose(id);
  }
}

void AudioplayersWebOsPlugin::dispose(const std::string& player_id) {
  bool unlocked = false;

  if (players_.find(player_id) != players_.end()) {
    bool last = (players_.size() == 1) ? true : false;
    if (last && isLocked()) {
      unlocked = true;
      platform_lock_.unlock();
    }
    AudioPlayer* pplayer = players_[player_id].get();
    pplayer->Dispose(last);

    debug_ids_.erase(player_id);
    players_.erase(player_id);
    historyErase(player_id);
  }

  forced_pauses_.erase(player_id);

  if (unlocked) {
    platform_lock_.lock();
  }
}

// Path utils...
std::string AudioplayersWebOsPlugin::headTrimmer(const std::string& path) {
  std::string str = path;

  if (0 == str.find("/")) {
    str = str.substr(1,str.size());
  } else if (0 == path.find("./")) {
    str = str.substr(2,str.size());
  }
  return str;
}

void AudioplayersWebOsPlugin::historyAdd(const std::string& player_id) {
  create_history_.push_back(player_id);

  size_t size = create_history_.size();
  if (size > MAX_PIPELINES) {
    size_t indx = size - MAX_PIPELINES;
    for (size_t i = 0; i < indx; i++) {
      std::string playerId = create_history_[i];
      AudioPlayer* player = getAudioPlayer(playerId);
      if (player) {
        player->OnError(AudioPlayer::ERROR_PLUGIN, "num of pipelines");
      }
    }
  }
}

void AudioplayersWebOsPlugin::historyErase(const std::string& player_id) {
  for (auto it = create_history_.begin(); it != create_history_.end(); ++it) {
    if (*it == player_id) {
      create_history_.erase(it);
      break;
    }
  }
}

}  // namespace

void AudioplayersSoloudPluginRegisterWithRegistrar(FlutterDesktopPluginRegistrarRef registrar) {
  AudioplayersWebOsPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
         ->GetRegistrar<flutter::PluginRegistrar>(registrar),
      FlutterDesktopPluginRegistrarGetView(registrar));
}

void WaitPlatform() {
  AudioplayersWebOsPlugin::WaitResumed();
}
