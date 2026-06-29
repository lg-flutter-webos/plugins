// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include <pbnjson.hpp>
#include "player.h"
#include "http.h"
#include "audio_player.h"
#include "log.h"

#include <flutter/event_stream_handler_functions.h>
#include <flutter/standard_method_codec.h>
#include <mutex>

#ifdef TAG
#undef TAG
#endif
#define TAG "AudioPlayer::"

static constexpr char STOP[] = "PlayerState.stopped";
static GMainContext* main_context_ = nullptr;

static std::string cut_url(const std::string& url) {
  size_t len = url.length();
  return len > 128
      ? "..." + url.substr(len - 125)
      : url;
}

/**********************************************/
/*                Soloud                      */
/**********************************************/
static Player player_;
static std::map<unsigned int, AudioPlayer*> handle_instances_;
static std::recursive_mutex soloud_mutex_;

static void add_handle(unsigned int handle, AudioPlayer* self) {
  std::lock_guard<std::recursive_mutex> autoLock(soloud_mutex_);
  handle_instances_[handle] = self;
}

static void remove_handle(unsigned int handle) {
  std::lock_guard<std::recursive_mutex> autoLock(soloud_mutex_);
  handle_instances_.erase(handle);
}

static AudioPlayer* get_audioplayer(unsigned int handle) {
  std::lock_guard<std::recursive_mutex> autoLock(soloud_mutex_);
  if (handle_instances_.find(handle) == handle_instances_.end()) {
    return nullptr;
  }
  return handle_instances_[handle];
}

/**********************************************/
/*                Http                        */
/**********************************************/
static Http http_;
static std::map<AudioPlayer*, std::string> http_listeners_;
static std::recursive_mutex http_mutex_;

static void add_http_listener(AudioPlayer* self, const std::string& url) {
  std::lock_guard<std::recursive_mutex> autoLock(http_mutex_);
  http_listeners_[self] = url;
}

static void remove_http_listener(AudioPlayer* self) {
  std::lock_guard<std::recursive_mutex> autoLock(http_mutex_);
  http_listeners_.erase(self);
}

std::map<AudioPlayer*, std::string> http_listeneres() {
  std::lock_guard<std::recursive_mutex> autoLock(http_mutex_);
  return http_listeners_;
}

bool has_http_listener(AudioPlayer* self) {
  std::lock_guard<std::recursive_mutex> autoLock(http_mutex_);
  return http_listeners_.find(self) == http_listeners_.end()
    ? false
    : true;
}

/**********************************************/
/*                AudioPlayer                 */
/**********************************************/
AudioPlayer::AudioPlayer(flutter::PluginRegistrar* plugin_registrar,
        const std::string& player_id, int debug_id)
  : player_id_(player_id)
  , debug_id_(debug_id) {

  if (main_context_ == nullptr) {
    main_context_ = g_main_context_ref_thread_default();
  }

  if (!player_.isInited()) {
    LogInfo(TAG) << "Soloud Player.init()";
    player_.init(44100, 2048, 2);
    player_.setVoiceEndedCallback([](unsigned int* handle) -> void {
      if (handle) {
        AudioPlayer* player = get_audioplayer(*handle);
        if (player) {
          attachToGMainLoop([](void* data) {
            AudioPlayer* self = (AudioPlayer*)data;
            self->OnPlayCompleted();
            return G_SOURCE_REMOVE;
          }, player);
        }
      }
    });
    player_.setGlobalVolume(1.0);
    player_.setMaxActiveVoiceCount(32);

    // initialize http
    http_.setDoneCallback([](const std::string& url) -> void {
      auto listeners = http_listeneres();
      for (auto it = listeners.begin(); it != listeners.end(); ++it) {
        if (it->second == url) {
          AudioPlayer* player = it->first;

          attachToGMainLoop([](void* data) {
            AudioPlayer* self = (AudioPlayer*)data;
            if (has_http_listener(self)) {
              remove_http_listener(self);
              self->onHttp();
            }
            return G_SOURCE_REMOVE;
          }, player);
        }
      }
    });
  }

  SetUpEventChannel(plugin_registrar->messenger());
}

AudioPlayer::~AudioPlayer() {
}

void AudioPlayer::loadCommon() {
  player_.play(hash_, handle_, volume_, balance_,
               request_play_
                 ? false : true,
               release_mode_ == ReleaseMode::kLoop
                 ? true : false,
               0.0);
  add_handle(handle_, this);

  OnInitialized();
}

void AudioPlayer::getHttpCommon() {
  uint8_t* data_source = nullptr;
  size_t data_size = 0;

  std::tie(data_source, data_size) = http_.get(uri_, (uint64_t)this);
  if (data_size > 0) {
    loadMem(data_source, data_size);
  } else {
    int error_code = 0;
    std::string error_text;
    std::tie(error_code, error_text) = http_.get_error(uri_);
    if (error_code != 0) {
      OnError(error_code, error_text);
    }
  }
}

void AudioPlayer::loadMem(uint8_t* buf, size_t buf_size) {
  PlayerErrors_t err = noError;

  if (opus_) {
    const unsigned int SAMPLE_RATE = 48000;
    const unsigned int CHANNELS = 2;

    PCMformat pcmFormat = { SAMPLE_RATE, CHANNELS, sizeof(float), OPUS};
    err = player_.setBufferStream(pcmFormat, hash_);
    if (err == noError) {
      player_.addAudioDataStream(hash_, buf, buf_size);
      player_.setDataIsEnded(hash_);
    }
  } else {
    err = player_.loadMem(uri_, buf, buf_size, low_latency_, hash_);
    if (err == fileAlreadyLoaded) {
      err = noError;
    }
  }

  if (err == noError) {
    loadCommon();
  } else {
    OnError(err, "loadMem error");
  }
}

void AudioPlayer::loadFile(const std::string& file_name) {
  PlayerErrors_t err = player_.loadFile(file_name, low_latency_, &hash_);
  if (err == noError || err == fileAlreadyLoaded) {
    loadCommon();
  } else {
    OnError(err, "soloud loadFile error");
  }
}

void AudioPlayer::SetUrl(const std::string &uri, const std::string& mime_type) {
  if (uri == uri_) {
    OnPrepared();
    return;
  }
  sink_player_state(STOP);
  Release();

  LogInfo(TAG) << "SetUrl(" << debug_id_ << ", " << cut_url(uri) << ")";
  uri_ = uri;
  if (mime_type.find("ogg") != std::string::npos &&
      mime_type.find("opus") != std::string::npos) {
    opus_ = true;
  }

  const std::string file_protocol_prefix = "file://";
  if (!opus_ && uri_.find(file_protocol_prefix) == 0) {
    std::string fname = uri_.substr(file_protocol_prefix.size());
    loadFile(fname);

    uri_ = fname;
  } else {
    add_http_listener(this, uri_);
  }

  getHttpCommon();
  OnPrepared();
}

void AudioPlayer::SetDataSource(std::vector<uint8_t> &data, const std::string& mime_type) {
  sink_player_state(STOP);
  Release();

  LogInfo(TAG) << "SetDataSource(" << debug_id_ << ", " << mime_type << ")";
  uri_ = player_id_;
  if (mime_type.find("ogg") != std::string::npos &&
      mime_type.find("opus") != std::string::npos) {
    opus_ = true;
  }

  size_t size = data.size();
  uint8_t* buf = (uint8_t*)::malloc(size);
  ::memcpy(buf, data.data(), size);

  data_source_ = buf;
  data_size_ = size;

  loadMem(buf, size);
  OnPrepared();
}

void AudioPlayer::Play() {
  request_play_ = true;
  if (uri_.empty()) {
    return;
  }

  if (handle_) {
    LogInfo(TAG) << "Play(" << debug_id_ << ")";
    player_.setPause(handle_, false);
  }
}

void AudioPlayer::Pause() {
  request_play_ = false;
  if (uri_.empty()) {
    return;
  }

  if (handle_) {
    LogInfo(TAG) << "Pause(" << debug_id_ << ")";
    player_.setPause(handle_, true);
  }
}

std::string AudioPlayer::releaseModeToString(ReleaseMode mode) {
  if (mode == ReleaseMode::kRelease) {
    return "kRelease";
  }
  if (mode == ReleaseMode::kLoop) {
    return "kLoop";
  }
  if (mode == ReleaseMode::kStop) {
    return "kStop";
  }
  return "kUnknown";
}

void AudioPlayer::SetReleaseMode(ReleaseMode mode) {
  LogInfo(TAG) << "SetReleaseMode(" << debug_id_ << ","
               << releaseModeToString(mode) << ")";
  if (release_mode_ == mode) {
    return;
  }
  if ((release_mode_ == ReleaseMode::kLoop ||
      mode == ReleaseMode::kLoop) && handle_) {
    player_.setLooping(handle_,
                       mode == ReleaseMode::kLoop
                         ? true : false);
  }
  release_mode_ = mode;
}

void AudioPlayer::SetVolume(double volume) {
  volume_ = volume;
  if (handle_) {
    player_.setVolume(handle_, volume);
  }
}

void AudioPlayer::SetPlaybackRate(double playback_rate) {
  if (handle_) {
    player_.setRelativePlaySpeed(handle_, playback_rate);
  }
}

void AudioPlayer::SetBalance(double balance) {
  balance_ = balance;
  if (handle_) {
    LogInfo(TAG) << "SetBalance " << balance;
    player_.setPan(handle_, balance);
  }
}

void AudioPlayer::Seek(int64_t position) {
  if (uri_.empty()) {
    LogInfo(TAG) << "player_set_play_position: empty url";
    return;
  }
  if (position != getCurrentPosition()) {
    seek_position_ = position;
    if (handle_) {
      player_.seek(handle_, position / 1000.);
      sink_seek_completed();
    }
  }
}

int64_t AudioPlayer::GetPosition() {
  return std::max(seek_position_, getCurrentPosition());
}

int64_t AudioPlayer::getCurrentPosition() {
  if (request_play_ && handle_) {
    current_position_ = player_.getPosition(handle_) * 1000;
  }
  return current_position_;
}

void AudioPlayer::Stop() {
  request_play_ = false;
  seek_position_ = 0;
  current_position_ = 0;

  if (handle_) {
    remove_handle(handle_);
    player_.stop(handle_);
    player_.removeHandle(handle_);
  }
  handle_ = 0;

  if (hash_) {
    player_.play(hash_, handle_, volume_, balance_,
                 true,
                 release_mode_ == ReleaseMode::kLoop
                   ? true : false,
                 0.0);
    add_handle(handle_, this);
  }
}

void AudioPlayer::Release() {
  // remove handle_;
  if (handle_) {
    remove_handle(handle_);
    player_.stop(handle_);
    player_.removeHandle(handle_);
  }
  handle_ = 0;

  // remove hash_
  if ((data_source_ || opus_) && hash_) {
    player_.disposeSound(hash_);
    hash_ = 0;
  }

  if (has_http_listener(this)) {
    remove_http_listener(this);
  }
  if (http_.erase(uri_,(uint64_t)this) == 0 && hash_) {
    player_.disposeSound(hash_);
    hash_ = 0;
  }
  hash_ = 0;

  // etc..
  pending_error_code_ = 0;
  pending_error_text_.clear();
  request_play_ = false;
  seek_position_ = 0;
  current_position_ = 0;

  is_initialized_ = false;
  if (data_source_) {
    ::free(data_source_);
  }
  data_source_ = nullptr;
  data_size_ = 0;
  uri_.clear();
  opus_ = false;

  volume_ = 1.0;
  balance_ = 0.0;
  duration_ = 0;
}

void AudioPlayer::Dispose(bool deep) {
  event_sink_ = nullptr;
  event_channel_->SetStreamHandler(nullptr);
  Release();

  if (deep) {
    LogInfo(TAG) << "Soloud Player.dispose()";
    player_.dispose();
  }
}

void AudioPlayer::SetLatencyMode(bool low_latency) {
  low_latency_ = low_latency;
}

int64_t AudioPlayer::GetDuration() {
  return duration_;
}

void AudioPlayer::SetUpEventChannel(flutter::BinaryMessenger *messenger) {
  std::string name =
      "xyz.luan/audioplayers/events/" + player_id_;
  auto channel =
      std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
          messenger, name, &flutter::StandardMethodCodec::GetInstance());
  auto handler = std::make_unique<
      flutter::StreamHandlerFunctions<flutter::EncodableValue>>(
      [&](const flutter::EncodableValue *arguments,
          std::unique_ptr<flutter::EventSink<>> &&events)
          -> std::unique_ptr<flutter::StreamHandlerError<>> {
        event_sink_ = std::move(events);
        FlushPendingEvents();
        return nullptr;
      },
      [&](const flutter::EncodableValue *arguments)
          -> std::unique_ptr<flutter::StreamHandlerError<>> {
        event_sink_ = nullptr;
        return nullptr;
      });
  channel->SetStreamHandler(std::move(handler));

  event_channel_ = std::move(channel);
}

void AudioPlayer::OnPrepared() {
  flutter::EncodableMap map = {{flutter::EncodableValue("event"),
                                flutter::EncodableValue("audio.onPrepared")},
                               {flutter::EncodableValue("value"),
                                flutter::EncodableValue(true)}};
  PushEvent(flutter::EncodableValue(map));
}

void AudioPlayer::OnInitialized() {
  if (!is_initialized_) {
    is_initialized_ = true;

    duration_ = player_.getLength(hash_) * 1000;
    LogInfo(TAG) << "OnInitialized(" << debug_id_
                 << ",duration:" << duration_ << ")";

    flutter::EncodableMap map = {{flutter::EncodableValue("event"),
                                  flutter::EncodableValue("audio.onDuration")},
                                 {flutter::EncodableValue("value"),
                                  flutter::EncodableValue(duration_)}};
    PushEvent(flutter::EncodableValue(map));
  }
}

void AudioPlayer::OnPlayCompleted() {
  switch (release_mode_) {
    case ReleaseMode::kLoop:
      break;

    case ReleaseMode::kStop:
      sink_play_completed();
      Stop();
      break;

    case ReleaseMode::kRelease:
      sink_play_completed();
      Release();
      break;

    default:
      break;
  }
}

void AudioPlayer::onHttp() {
  getHttpCommon();
}

void AudioPlayer::OnError(int64_t code, const std::string& text) {
  LogError(TAG) << "OnError(" << debug_id_
      << "," << code
      << "," << text << ")";

  sink_player_state(STOP);
  Release();

  if (event_sink_ == nullptr) {
    pending_error_code_ = code;
    pending_error_text_ = text;
    return;
  }
  pending_error_code_ = 0;
  pending_error_text_.clear();

  event_sink_->Error(std::to_string(code), text);
}

void AudioPlayer::sink_seek_completed() {
  flutter::EncodableMap map = {{flutter::EncodableValue("event"),
                                flutter::EncodableValue("audio.onSeekComplete")},
                               {flutter::EncodableValue("value"),
                                flutter::EncodableValue(true)}};
  PushEvent(flutter::EncodableValue(map));
}

void AudioPlayer::sink_play_completed() {
  LogInfo(TAG) << "OnPlayeCompleted(" << debug_id_ << ")";
  flutter::EncodableMap map = {{flutter::EncodableValue("event"),
                                flutter::EncodableValue("audio.onComplete")},
                               {flutter::EncodableValue("value"),
                                flutter::EncodableValue(true)}};
  PushEvent(flutter::EncodableValue(map));
}

void AudioPlayer::sink_player_state(const std::string& state) {
  flutter::EncodableMap map = {{flutter::EncodableValue("event"),
                                flutter::EncodableValue("audio.onLog")},
                               {flutter::EncodableValue("value"),
                                flutter::EncodableValue(state)}};
  PushEvent(flutter::EncodableValue(map));
}

void AudioPlayer::PushEvent(const flutter::EncodableValue& value) {
  if (event_sink_) {
    event_sink_->Success(value);
  } else {
    pending_events_.push_back(value);
  }
}

void AudioPlayer::FlushPendingEvents() {
  if (event_sink_) {
    for (auto& value : pending_events_) {
      event_sink_->Success(value);
    }
    pending_events_.clear();

    if (pending_error_code_) {
      OnError(pending_error_code_, std::string(pending_error_text_));
    }
  }
}

GSource* AudioPlayer::attachToGMainLoop(GMainLoopCallback cb, void* data, int64_t msec) {
  GSource* src = (msec > 0) ?  g_timeout_source_new(msec) : g_idle_source_new();
  g_source_set_callback(src,cb,data,NULL);
  g_source_attach(src,main_context_);
  g_source_unref(src);
  return src;
}

