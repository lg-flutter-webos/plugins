// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "include/audioplayers_webos/audioplayers_web_os_plugin.h"

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

#include <pbnjson.hpp>
#include "audio_player.h"
#include "log.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "AudioplayersWebOsPlugin::"

static const size_t MAX_PIPELINES = 5;
static constexpr char kSendVisibilityStatusMethodName[] = "visibilityStatus";
static const char *kInvalidArgument = "Invalid argument";

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
  std::map<std::string, std::unique_ptr<AudioPlayer>> disposed_players_;
  std::vector<std::string> disposed_player_ids_;
  std::map<std::string, bool> forced_pauses_;
  std::map<std::string, int> debug_ids_;
  std::vector<std::string> create_history_;
  flutter::PluginRegistrar *registrar_;
  std::string app_id_;
  bool background_run_ = false;
  bool visibility_ = false;
};

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

AudioplayersWebOsPlugin::AudioplayersWebOsPlugin(
    flutter::PluginRegistrar *registrar,
    FlutterDesktopViewRef view)
  : registrar_(registrar) {
  app_id_ = ::getenv("FLUTTER_APP_ID");

  bool webos_app = (app_id_.find("com.webos.") == 0) ? true : false;
  if (webos_app) {
    std::string path = ::getenv("FLUTTER_BUNDLE_PATH");
    path += "/appinfo.json";
    pbnjson::JValue doc = pbnjson::JDomParser::fromFile(path.c_str());
    background_run_ = doc["enableBackgroundRun"].isBoolean()
                    ? doc["enableBackgroundRun"].asBool()
                    : false;
  }
  FlutterDesktopViewAddListener(
      view,
      kSendVisibilityStatusMethodName,
      std::bind(&AudioplayersWebOsPlugin::handleVisibility, this, std::placeholders::_1));
}

void AudioplayersWebOsPlugin::handleVisibility(const std::string& response) {
  pbnjson::JValue doc = pbnjson::JDomParser::fromString(response);

  bool visibility = doc["visibility"].asBool();
  if (visibility_ == visibility) {
    return;
  }

  visibility_ = visibility;
  LogInfo(TAG) << "visibility: " << visibility_;
  if (!visibility) {
    forced_pauses_.clear();
  }

  for (std::string& playerId : create_history_) {
    AudioPlayer* player = getAudioPlayer(playerId);
    if (player == nullptr) {
      continue;
    }

    player->SetVisibility(visibility);

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

    // If the slot capacity is reached, evict the least-recently-created
    // player synchronously so its slot is freed before the new player
    // tries to acquire one.
    if (players_.size() >= MAX_PIPELINES) {
      std::string oldest_id;
      while (!create_history_.empty()) {
        oldest_id = create_history_.front();
        if (players_.find(oldest_id) != players_.end()) break;
        create_history_.erase(create_history_.begin());
        oldest_id.clear();
      }
      if (!oldest_id.empty()) {
        if (AudioPlayer* oldest = getAudioPlayer(oldest_id)) {
          oldest->OnError(AudioPlayer::ERROR_PLUGIN, "num of pipelines");
        }
        debug_ids_.erase(oldest_id);
        players_.erase(oldest_id);
        historyErase(oldest_id);
        forced_pauses_.erase(oldest_id);
        LogInfo(TAG) << "Create() evicted " << oldest_id
                     << " to free a slot";
      }
    }

    std::unique_ptr<AudioPlayer> audio_player;
    try {
      audio_player = std::make_unique<AudioPlayer>(registrar_,
                                                   player_id, debug_id,
                                                   visibility_);
    } catch (const std::runtime_error& error) {
      // Defensive fallback; should not be reached because of the eviction
      // above. Surfaced if a slot somehow remains unavailable.
      LogError(TAG) << "Create() failed: " << error.what();
      return;
    }

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
    player->Release(false);
  } else if (method_name == "seek") {
    result->Success();
    player->Seek(GetRequiredArg<int32_t>(arguments, "position"));
  } else if (method_name == "setVolume" || method_name == "setBalance") {
    // Volume and balance control are not supported on webOS in this
    // release. Surfaced to Dart as MissingPluginException; see README.
    result->NotImplemented();
    return;
  } else if (method_name == "setPlaybackRate") {
    result->Success();
    double rate = GetRequiredArg<double>(arguments, "playbackRate");
    player->SetPlaybackRate(rate);
  } else if (method_name == "setReleaseMode") {
    result->Success();
    std::string release_mode =
        GetRequiredArg<std::string>(arguments, "releaseMode");
    player->SetReleaseMode(StringToReleaseMode(release_mode));
  } else if (method_name == "getDuration") {
    result->Success(flutter::EncodableValue(player->GetDuration()));
  } else  if (method_name == "getCurrentPosition") {
    result->Success(flutter::EncodableValue(player->GetPosition()));
  } else if (method_name == "setPlayerMode") {
    result->Success();
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

AudioplayersWebOsPlugin::~AudioplayersWebOsPlugin() {
  disposeAllPlayers();
}

void AudioplayersWebOsPlugin::disposeAllPlayers() {
  std::vector<std::string> ids = create_history_;
  for (std::string& id : ids) {
    dispose(id);
  }
}

void AudioplayersWebOsPlugin::dispose(const std::string& player_id) {
  if (players_.find(player_id) != players_.end()) {
    AudioPlayer* pplayer = players_[player_id].get();
    disposed_players_[player_id] = std::move(players_[player_id]);
    debug_ids_.erase(player_id);
    players_.erase(player_id);
    historyErase(player_id);

    pplayer->Dispose([this](const std::string& player_id) -> void {
      disposed_player_ids_.push_back(player_id);

      // Delay actual destruction so any in-flight Luna messages from the
      // platform media subsystem can drain before the underlying media
      // client is torn down — otherwise the message thread dereferences a
      // freed vtable on the just-destroyed client.
      WebOSPlayer::attachToGMainLoop([](void* data) -> int {
        AudioplayersWebOsPlugin* self = (AudioplayersWebOsPlugin*)data;
        for (std::string& player_id : self->disposed_player_ids_) {
          self->disposed_players_.erase(player_id);
        }
        self->disposed_player_ids_.clear();
        return G_SOURCE_REMOVE;
      }, this, 2000);
    });
  }

  forced_pauses_.erase(player_id);
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
  // Eviction when the slot pool is exhausted is handled synchronously in
  // the create branch of handleMethodCall; this is just an append.
  create_history_.push_back(player_id);
}

void AudioplayersWebOsPlugin::historyErase(const std::string& player_id) {
  for (auto it = create_history_.begin(); it != create_history_.end(); ++it) {
    if (*it == player_id) {
      create_history_.erase(it);
      break;
    }
  }
}

void AudioplayersWebOsPluginRegisterWithRegistrar(FlutterDesktopPluginRegistrarRef registrar) {
  AudioplayersWebOsPlugin::RegisterWithRegistrar(
   flutter::PluginRegistrarManager::GetInstance()
         ->GetRegistrar<flutter::PluginRegistrar>(registrar),
      FlutterDesktopPluginRegistrarGetView(registrar));
}
