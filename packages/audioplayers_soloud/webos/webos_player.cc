// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "webos_player.h"

#include <NDL/NDL_media.h>
#include <pbnjson.hpp>

#include <algorithm>

#include "log.h"
#include "plugin_player_delegate.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "WebOSPlayer::"

namespace {

// Slot 0 is the only one this plugin uses.
constexpr int kSlot = 0;

class JsonWriter {
 public:
  void StartObject() {
    if (!start_) string_buf_ += ",";
    string_buf_ += "{";
    start_ = true;
  }
  void Key(const std::string& key) {
    if (!start_) string_buf_ += ",";
    string_buf_ += "\"" + key + "\":";
    start_ = true;
  }
  void String(const std::string& value) {
    if (!start_) string_buf_ += ",";
    string_buf_ += "\"" + value + "\"";
    start_ = false;
  }
  void Bool(bool set) {
    if (!start_) string_buf_ += ",";
    string_buf_ += set ? "true" : "false";
    start_ = false;
  }
  void Int64(int64_t v) {
    if (!start_) string_buf_ += ",";
    string_buf_ += std::to_string(v);
    start_ = false;
  }
  void EndObject() { string_buf_ += "}"; }
  std::string GetString() { return string_buf_; }

 private:
  std::string string_buf_;
  bool start_ = true;
};

}  // namespace

GMainContext* WebOSPlayer::s_main_context_ = nullptr;
WebOSPlayer* WebOSPlayer::active_instance_ = nullptr;
bool WebOSPlayer::slot_initialized_ = false;

std::mutex& WebOSPlayer::MediaCallMutex() {
  static std::mutex m;
  return m;
}

void WebOSPlayer::OnSlotEvent(char* json) {
  WebOSPlayer* owner = nullptr;
  {
    std::lock_guard<std::mutex> lock(MediaCallMutex());
    owner = active_instance_;
  }
  if (owner != nullptr && json != nullptr) {
    owner->EnqueueRawEvent(json);
  }
}

WebOSPlayer::WebOSPlayer(PluginPlayerDelegate* delegate)
    : delegate_(delegate),
      visibility_(false),
      speed_(1.0),
      audio_out_(true),
      state_(UNLOADED),
      current_(0),
      duration_(0),
      seekable_(false),
      has_video_(false),
      has_audio_(false),
      videoW_(0),
      videoH_(0),
      buffering_(false),
      buffering_start_(0),
      buffering_end_(0) {
  if (s_main_context_ == nullptr) {
    s_main_context_ = g_main_context_ref_thread_default();
  }

  // Initialise the platform slot once for the process lifetime. Init +
  // Subscribe are kept alive across instances because the corresponding
  // Quit unregisters a global app-state hook with cross-slot side
  // effects on other plugins.
  std::lock_guard<std::mutex> lock(MediaCallMutex());
  active_instance_ = this;

  if (NDL_MediaSetPlayer(kSlot) != 0) {
    LogError(TAG) << "select player(" << kSlot
                  << ") failed: " << NDL_MediaGetError();
    return;
  }
  if (!slot_initialized_) {
    LogInfo(TAG) << "slot[" << kSlot << "] NDL_MediaInit (first use)";
    if (NDL_MediaInit(0) != 0) {
      LogError(TAG) << "slot[" << kSlot
                    << "] NDL_MediaInit failed: " << NDL_MediaGetError();
      return;
    }
    if (NDL_MediaSubscribeEvents(&WebOSPlayer::OnSlotEvent) != 0) {
      LogError(TAG) << "slot[" << kSlot
                    << "] NDL_MediaSubscribeEvents failed: "
                    << NDL_MediaGetError();
      return;
    }
    slot_initialized_ = true;
    LogInfo(TAG) << "slot[" << kSlot << "] init complete";
  } else {
    LogInfo(TAG) << "slot[" << kSlot << "] reusing existing init";
    // A previous instance's dtor calls NDL_MediaUnsubscribeEvents to
    // make sure no platform callback survives the plugin's dlclose, so
    // we have to re-register the trampoline here. Init itself remains
    // alive for the process lifetime.
    if (NDL_MediaSubscribeEvents(&WebOSPlayer::OnSlotEvent) != 0) {
      LogWarn(TAG) << "slot[" << kSlot
                   << "] re-subscribe failed (likely already subscribed): "
                   << NDL_MediaGetError();
    }
    NDL_MediaUnload();
  }
}

WebOSPlayer::~WebOSPlayer() {
  // Lifecycle guard: any GSource or platform callback that fires after
  // teardown must observe run_ == false and skip work.
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    run_ = false;
  }
  if (idle_source_) g_source_destroy(idle_source_);
  if (timer_source_) g_source_destroy(timer_source_);

  // The plugin shared library is about to be unmapped (dlclose on app
  // teardown). Drop the static OnSlotEvent registration first so the
  // platform's background thread cannot jump into freshly unmapped
  // code while flushing pending events.
  //
  // We deliberately skip NDL_MediaUnload here: the owner (DummyPlayer)
  // already calls unload(true) before destruction, and any new in-flight
  // unloadCompleted event would race teardown without buying anything.
  // NDL_MediaQuit is also avoided on purpose — it unregisters a global
  // app-state hook with cross-slot side effects on other plugins.
  std::lock_guard<std::mutex> lock(MediaCallMutex());
  if (active_instance_ == this) {
    active_instance_ = nullptr;
  }
  if (NDL_MediaSetPlayer(kSlot) == 0) {
    NDL_MediaUnsubscribeEvents();
  }
  LogInfo(TAG) << "slot[" << kSlot << "] unsubscribed (~WebOSPlayer)";
  // slot_initialized_ stays true so a future WebOSPlayer in the same
  // process re-uses the existing slot init (NDL_MediaInit is invoked at
  // most once per process). The reuse branch in the ctor re-subscribes
  // the event handler.
}

std::string WebOSPlayer::BuildLoadPayload(const std::string& appId,
                                          const std::string& winId,
                                          const std::string& uri,
                                          const std::string& hint,
                                          int64_t start_position) {
  std::string media_transport_type;
  if (hint.find("MPEG-DASH") != std::string::npos) {
    media_transport_type = "MPEG-DASH";
  } else if (hint.find("HLS") != std::string::npos) {
    media_transport_type = "HLS";
  } else if (uri.find("m3u8") != std::string::npos) {
    media_transport_type = "HLS";
  } else if (uri.find("udp://") != std::string::npos) {
    media_transport_type = "UDP";
  } else if (uri.find("rtp://") != std::string::npos) {
    media_transport_type = "RTP";
  } else if (uri.find("rtsp://") != std::string::npos) {
    media_transport_type = "RTSP";
  }

  // If the caller already supplied an option JSON in `hint`, splice the
  // computed appId / windowId / startPosition into its "option" object.
  std::string payload;
  bool merged = false;

  pbnjson::JValue doc = pbnjson::JDomParser::fromString(hint);
  if (doc.isObject()) {
    pbnjson::JGenerator serializer(nullptr);
    std::string serialized;
    if (serializer.toString(doc, pbnjson::JSchema::AllSchema(), serialized)) {
      const std::string option_header = "\"option\":{";
      auto option_start = serialized.find(option_header);
      if (option_start != std::string::npos) {
        merged = true;

        JsonWriter writer;
        if (!doc["option"]["appId"].isString()) {
          writer.Key("appId");
          writer.String(appId);
        }
        if (!winId.empty()) {
          writer.Key("windowId");
          writer.String(winId);
        }
        if (start_position > 0 &&
            !doc["option"]["transmission"].isObject()) {
          writer.Key("transmission");
          writer.StartObject();
            writer.Key("playTime");
            writer.StartObject();
              writer.Key("start");
              writer.Int64(start_position);
            writer.EndObject();
          writer.EndObject();
        }
        std::string tmp = writer.GetString();
        if (!tmp.empty()) {
          tmp += ",";
        }
        payload = serialized;
        payload.insert(option_start + option_header.size(), tmp);
      }
    }
  }

  if (!merged) {
    JsonWriter writer;
    writer.StartObject();
      if (!media_transport_type.empty()) {
        writer.Key("mediaTransportType");
        writer.String(media_transport_type);
      }

      writer.Key("option");
      writer.StartObject();
        writer.Key("appId");
        writer.String(appId);
        if (hint.find("lowDelayMode") != std::string::npos) {
          writer.Key("lowDelayMode");
          writer.Bool(true);
        }
        if (!winId.empty()) {
          writer.Key("windowId");
          writer.String(winId);
        }
        if (start_position > 0) {
          writer.Key("transmission");
          writer.StartObject();
            writer.Key("playTime");
            writer.StartObject();
              writer.Key("start");
              writer.Int64(start_position);
            writer.EndObject();
          writer.EndObject();
        }
      writer.EndObject();
    writer.EndObject();
    payload = writer.GetString();
  }
  return payload;
}

bool WebOSPlayer::load(const std::string& appId,
                       const std::string& winId,
                       const std::string& uri,
                       const std::string& hint,
                       int64_t start_position) {
  loadParams_.appId = appId;
  loadParams_.winId = winId;
  loadParams_.uri = uri;
  loadParams_.hint = hint;
  loadParams_.start_position = start_position;

  if (state_ != UNLOADED) {
    LogWarn(TAG) << "currentState is " << stateToString(state_)
                 << ", I will unload pipeline";
    unload(true);
  }
  if (start_position != 0) {
    LogInfo(TAG) << "load(startPosition:" << start_position << ")";
  }
  current_ = start_position;

  std::string payload =
      BuildLoadPayload(appId, winId, uri, hint, start_position);

  LogInfo(TAG) << "media url is " << uri;
  LogInfo(TAG) << "load payload is " << payload;

  std::lock_guard<std::mutex> lock(MediaCallMutex());
  LogInfo(TAG) << "slot[" << kSlot << "] load: NDL_MediaSetPlayer";
  if (NDL_MediaSetPlayer(kSlot) != 0) {
    LogError(TAG) << "slot[" << kSlot << "] NDL_MediaSetPlayer failed: "
                  << NDL_MediaGetError();
    return false;
  }
  if (NDL_MediaLoadWithOption(uri.c_str(), payload.c_str()) != 0) {
    LogError(TAG) << "slot[" << kSlot
                  << "] NDL_MediaLoadWithOption failed: "
                  << NDL_MediaGetError();
    return false;
  }

  state_ = IN_LOADING;
  delegate_->OnStateChanged(state_);
  return true;
}

bool WebOSPlayer::unload(bool reset) {
  bool retv = true;
  if (state_ != UNLOADED) {
    {
      std::lock_guard<std::mutex> lock(MediaCallMutex());
      if (NDL_MediaSetPlayer(kSlot) != 0 || NDL_MediaUnload() != 0) {
        LogWarn(TAG) << "unload() at " << stateToString(state_) << " failed";
        retv = false;
      }
    }

    if (reset) {
      current_ = 0;
      duration_ = 0;
      seekable_ = false;
      has_video_ = false;
      has_audio_ = false;
      videoW_ = 0;
      videoH_ = 0;
      buffering_ = false;
      buffering_start_ = 0;
      buffering_end_ = 0;
    }

    speed_ = 1.0;
    audio_out_ = true;

    state_ = UNLOADED;
    delegate_->OnStateChanged(state_);
  }
  return retv;
}

bool WebOSPlayer::play() {
  if (state_ == UNLOADED || state_ == IN_LOADING || state_ == REMOVED) {
    return true;
  }
  std::lock_guard<std::mutex> lock(MediaCallMutex());
  if (NDL_MediaSetPlayer(kSlot) != 0 || NDL_MediaPlay() != 0) {
    LogWarn(TAG) << "play() at " << stateToString(state_) << " failed";
    return false;
  }
  return true;
}

bool WebOSPlayer::pause() {
  if (state_ == UNLOADED || state_ == IN_LOADING || state_ == REMOVED) {
    return true;
  }
  std::lock_guard<std::mutex> lock(MediaCallMutex());
  if (NDL_MediaSetPlayer(kSlot) != 0 || NDL_MediaPause() != 0) {
    LogWarn(TAG) << "pause() at " << stateToString(state_) << " failed";
    return false;
  }
  return true;
}

bool WebOSPlayer::seek(int64_t position, bool update_position) {
  if (!seekable_ && position != 0) {
    LogInfo(TAG) << "can't seek, because it's not a seekable contents";
    return false;
  }
  if (state_ == UNLOADED || state_ == IN_LOADING || state_ == REMOVED) {
    return false;
  }
  {
    std::lock_guard<std::mutex> lock(MediaCallMutex());
    if (NDL_MediaSetPlayer(kSlot) != 0 || NDL_MediaSeekTo(position) != 0) {
      LogWarn(TAG) << "seek() at " << stateToString(state_) << " failed";
      return false;
    }
  }
  if (update_position) current_ = position;
  return true;
}

bool WebOSPlayer::setPlayRate(double playRate, bool audioOut) {
  if (playRate == speed_ && audioOut == audio_out_) {
    return true;
  }
  LogInfo(TAG) << "setPlayRate(" << playRate << ", audioOut:" << audioOut
               << ")";
  speed_ = playRate;
  audio_out_ = audioOut;

  if (state_ == UNLOADED || state_ == IN_LOADING || state_ == REMOVED) {
    return true;
  }
  std::lock_guard<std::mutex> lock(MediaCallMutex());
  if (NDL_MediaSetPlayer(kSlot) != 0 ||
      NDL_MediaSetPlaybackRate(playRate) != 0) {
    LogWarn(TAG) << "setPlayRate(" << speed_ << "," << audio_out_
                 << ") at " << stateToString(state_) << " failed";
    return false;
  }
  return true;
}

bool WebOSPlayer::notifyVisibility(bool visibility) {
  // Foreground/background transitions are handled automatically by the
  // platform layer via the system app-state hook. We only track the
  // local flag so state queries are consistent.
  visibility_ = visibility;
  return true;
}

int64_t WebOSPlayer::getCurrentPosition() {
  std::lock_guard<std::mutex> lock(MediaCallMutex());
  if (NDL_MediaSetPlayer(kSlot) != 0) return current_;
  long long pos = NDL_MediaGetCurrentPosition();
  if (pos >= 0) current_ = pos;
  return current_;
}

void WebOSPlayer::EnqueueRawEvent(const char* json) {
  if (!json) return;
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!run_) return;
  event_queue_.emplace_back(json);
  if (idle_source_ == nullptr) {
    idle_source_ = attachToGMainLoop(
        [](void* data) -> int {
          auto* self = static_cast<WebOSPlayer*>(data);
          self->DrainEventQueue();
          return G_SOURCE_REMOVE;
        },
        this);
  }
}

void WebOSPlayer::DrainEventQueue() {
  std::vector<std::string> events;
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    idle_source_ = nullptr;
    if (!run_) return;
    events.swap(event_queue_);
  }
  for (auto& json : events) {
    HandleEvent(json);
  }
}

void WebOSPlayer::HandleEvent(const std::string& json) {
  pbnjson::JValue doc = pbnjson::JDomParser::fromString(json);
  if (!doc.isObject()) {
    LogWarn(TAG) << "HandleEvent: failed to parse JSON: " << json;
    return;
  }

  if (doc["loadCompleted"].isObject() ||
      doc["preloadCompleted"].isObject()) {
    state_ = LOADED;
    notifyVisibility(visibility_);
    if (speed_ != 1.0) {
      std::lock_guard<std::mutex> lock(MediaCallMutex());
      if (NDL_MediaSetPlayer(kSlot) == 0) {
        NDL_MediaSetPlaybackRate(speed_);
      }
    }
    delegate_->OnStateChanged(LOADED);
    return;
  }
  if (doc["playing"].isObject()) {
    state_ = PLAYING;
    delegate_->OnStateChanged(PLAYING);
    return;
  }
  if (doc["paused"].isObject()) {
    state_ = PAUSED;
    delegate_->OnStateChanged(PAUSED);
    return;
  }
  if (doc["seekDone"].isObject()) {
    delegate_->OnSeekCompleted();
    return;
  }
  if (doc["bufferingStart"].isObject()) {
    if (!buffering_) {
      buffering_ = true;
      delegate_->OnBufferingStart();
    }
    return;
  }
  if (doc["bufferingEnd"].isObject()) {
    if (buffering_) {
      buffering_ = false;
      delegate_->OnBufferingEnd();
    }
    return;
  }
  if (doc["endOfStream"].isObject()) {
    const int64_t MAX_WAIT = 1000;
    if (timer_source_) g_source_destroy(timer_source_);
    int64_t delay = std::min(duration_ - current_.load(), MAX_WAIT);
    if (delay < 0) delay = 0;
    timer_source_ = attachToGMainLoop(
        [](void* data) -> int {
          auto* self = static_cast<WebOSPlayer*>(data);
          {
            std::lock_guard<std::recursive_mutex> lock(self->mutex_);
            self->timer_source_ = nullptr;
            if (!self->run_) return G_SOURCE_REMOVE;
          }
          self->state_ = EOS;
          self->current_ = self->duration_;
          self->delegate_->OnStateChanged(EOS);
          self->delegate_->OnPlayCompleted();
          return G_SOURCE_REMOVE;
        },
        this, delay);
    return;
  }
  if (doc["currentTime"].isObject()) {
    pbnjson::JValue p = doc["currentTime"];
    if (p["currentTime"].isNumber()) {
      int64_t cur = 0;
      p["currentTime"].asNumber<int64_t>(cur);
      current_ = cur;
    }
    return;
  }
  if (doc["sourceInfo"].isObject()) {
    pbnjson::JValue p = doc["sourceInfo"];

    // Skip the placeholder sourceInfo the platform emits before programInfo
    // is populated. Without this guard a premature OnInitialized fires
    // with duration:0 and zero track counts, latching is_initialized_
    // and swallowing the real event that follows.
    pbnjson::JValue programs = p["programInfo"];
    if (!programs.isArray() || programs.arraySize() == 0) {
      return;
    }
    pbnjson::JValue prog = programs[0];
    int64_t dur = 0;
    int32_t num_video = 0, num_audio = 0;
    if (prog["duration"].isNumber()) prog["duration"].asNumber<int64_t>(dur);
    if (prog["numVideoTracks"].isNumber()) {
      prog["numVideoTracks"].asNumber<int32_t>(num_video);
    }
    if (prog["numAudioTracks"].isNumber()) {
      prog["numAudioTracks"].asNumber<int32_t>(num_audio);
    }
    if (dur == 0 && num_video == 0 && num_audio == 0) {
      return;
    }

    if (p["seekable"].isBoolean()) seekable_ = p["seekable"].asBool();
    duration_ = dur;
    has_audio_ = (num_audio > 0);
    has_video_ = (num_video > 0);

    LogInfo(TAG) << "sourceInfo WH[" << videoW_ << "," << videoH_
                 << "],duration:" << duration_ / 1000
                 << ",seekable:" << seekable_
                 << ",hasAudio:" << has_audio_
                 << ",hasVideo:" << has_video_;
    delegate_->OnInitialized();
    return;
  }
  if (doc["videoInfo"].isObject()) {
    pbnjson::JValue p = doc["videoInfo"];
    if (p["width"].isNumber()) p["width"].asNumber<int32_t>(videoW_);
    if (p["height"].isNumber()) p["height"].asNumber<int32_t>(videoH_);
    return;
  }
  if (doc["error"].isObject()) {
    pbnjson::JValue p = doc["error"];
    int64_t code = 0;
    if (p["errorCode"].isNumber()) p["errorCode"].asNumber<int64_t>(code);
    HandleErrorEvent(code);
    return;
  }
  if (doc["unloadCompleted"].isObject()) {
    state_ = UNLOADED;
    delegate_->OnStateChanged(UNLOADED);
    return;
  }
  // Ignore: validData, externalSubtitleTrackInfo, audioInfo, seekableRanges
}

void WebOSPlayer::HandleErrorEvent(int64_t code) {
  LogInfo(TAG) << "onError(" << code << ") at " << stateToString(state_);
  if (code == SMP_SEEK_FAILURE) {
    return;
  }
  if (code == SMP_RM_RELATED_ERROR) {
    state_ = REMOVED;
    delegate_->OnStateChanged(REMOVED);
    return;
  }
  if (code == SMP_MEDIA_API_PLAYING_ERROR) {
    if (state_ == IN_LOADING) {
      {
        std::lock_guard<std::mutex> lock(MediaCallMutex());
        if (NDL_MediaSetPlayer(kSlot) == 0) NDL_MediaUnload();
      }
      state_ = UNLOADED;
      LoadParams& p = loadParams_;
      load(p.appId, p.winId, p.uri, p.hint, p.start_position);
    } else {
      std::lock_guard<std::mutex> lock(MediaCallMutex());
      if (NDL_MediaSetPlayer(kSlot) == 0) NDL_MediaSeekTo(0);
    }
    return;
  }
  delegate_->OnError(code, "");
}

GSource* WebOSPlayer::attachToGMainLoop(GMainLoopCallback cb, void* data,
                                       int64_t msec) {
  GSource* src =
      (msec > 0) ? g_timeout_source_new(msec) : g_idle_source_new();
  g_source_set_callback(src, cb, data, NULL);
  g_source_attach(src, s_main_context_);
  g_source_unref(src);
  return src;
}

std::string WebOSPlayer::stateToString(int state) {
  switch (state) {
    case UNLOADED:
      return "UNLOADED";
    case IN_LOADING:
      return "IN_LOADING";
    case LOADED:
      return "LOADED";
    case PLAYING:
      return "PLAYING";
    case PAUSED:
      return "PAUSED";
    case EOS:
      return "EOF";
    case REMOVED:
      return "REMOVED";
    default:
      return "UNKNOWN";
  }
}
