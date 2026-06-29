// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include <pbnjson.hpp>
#include "audio_player.h"
#include "log.h"

#include <flutter/event_stream_handler_functions.h>
#include <flutter/standard_method_codec.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifdef TAG
#undef TAG
#endif
#define TAG "AudioPlayer::"

static constexpr char STOP[] = "PlayerState.stopped";
static constexpr char PAUSE[] = "PlayerState.paused";

static std::string cut_url(const std::string& url) {
  size_t len = url.length();
  return len > 128
      ? "..." + url.substr(len - 125)
      : url;
}

AudioPlayer::AudioPlayer(flutter::PluginRegistrar* plugin_registrar,
        const std::string& player_id, int debug_id, const bool visibility)
    : player_id_(player_id)
    , debug_id_(debug_id)
    , player_(WebOSPlayer::Create(this)) {
  if (!player_) {
    // Surfaced when all player slots are in use; the plugin handler is
    // expected to evict the oldest player and retry.
    throw std::runtime_error("MAX_PLAYERS_EXCEEDED");
  }
  SetVisibility(visibility);
  app_id_ = ::getenv("FLUTTER_APP_ID");

  SetUpEventChannel(plugin_registrar->messenger());
}

AudioPlayer::~AudioPlayer() {
  LogInfo(TAG) << "Destroyed(" << debug_id_ << ")";
  if (dispose_timeout_source_) {
    g_source_destroy(dispose_timeout_source_);
    dispose_timeout_source_ = nullptr;
  }
  Release();
}

void AudioPlayer::loadCommon() {
  player_->load(app_id_, "", uri_, mime_type_, "", seek_position_);
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
  if (!mime_type.empty()) {
    pbnjson::JValue doc = pbnjson::JDomParser::fromString(mime_type);
    if (doc.isObject()) {
      mime_type_ = mime_type;
    }
  }

  loadCommon();
  OnPrepared();
}

void AudioPlayer::SetDataSource(std::vector<uint8_t> &data, const std::string& mime_type) {
  sink_player_state(STOP);
  Release();

  LogInfo(TAG) << "SetDataSource(" << debug_id_ << ", size: " << data.size() << ")";
  std::string path = "/tmp/";
  path += app_id_;
  if (!std::filesystem::exists(path)) {
    std::filesystem::create_directories(path);
  }

  path += "/";
  path += player_id_;
  std::ofstream data_file(path, std::ios::binary);
  if (data_file.is_open()) {
    data_file.write((const char*)data.data(), data.size());
    data_file.close();
  }

  const std::string file_protocol_prefix = "file://";
  uri_ = file_protocol_prefix + path;
  if (!mime_type.empty()) {
    pbnjson::JValue doc = pbnjson::JDomParser::fromString(mime_type);
    if (doc.isObject()) {
      mime_type_ = mime_type;
    }
  }

  loadCommon();
  OnPrepared();
}

void AudioPlayer::Play() {
  request_play_ = true;
  if (uri_.empty()) {
    return;
  }
  if (reload_) {
    return;
  }

  if (removed_) {
    removed_ = false;
    reload_ = true;
    loadCommon();
  } else {
    player_->play();
  }
}

void AudioPlayer::Pause() {
  request_play_ = false;
  if (uri_.empty()) {
    return;
  }
  if (removed_ || reload_) {
    return;
  }

  player_->pause();
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
  release_mode_ = mode;
}

void AudioPlayer::SetPlaybackRate(double playback_rate) {
  player_->setPlayRate(playback_rate, true);
}

void AudioPlayer::Seek(int64_t position) {
  if (uri_.empty()) {
    LogInfo(TAG) << "player_set_play_position: empty url";
    return;
  }
  if (!player_->isSeekable()) {
    LogInfo(TAG) << "player_set_play_position: not seekable content";
    return;
  }

  if (position == player_->getCurrentPosition()) {
    seek_position_ = position;
  } else if (reload_ || removed_) {
    seek_position_ = position;
    sink_seek_completed();
  } else if (player_->seek(position, true)) {
    seek_position_ = position;
    sink_seek_completed();
  }
}

int64_t AudioPlayer::GetPosition() {
  int64_t position = player_->getCurrentPosition();
  return std::max(seek_position_, position);
}

void AudioPlayer::Stop() {
  Pause();

  seek_position_ = 0;
  player_->seek(seek_position_, true);
}

void AudioPlayer::Release(bool deep) {
  player_->unload(deep);

  pending_error_code_ = 0;
  pending_error_text_.clear();
  request_play_ = false;
  seek_position_ = 0;
  reload_ = false;

  if (deep) {
    is_initialized_ = false;
    uri_.clear();
    removeDataSource();
    mime_type_.clear();
    removed_ = false;
  } else {
    removed_ = true;
  }
}

void AudioPlayer::Dispose(const DisposedCallback disposed_cb) {
  disposed_cb_ = disposed_cb;
  event_sink_ = nullptr;
  event_channel_->SetStreamHandler(nullptr);

  int64_t timeout = (player_->getState() ==
                     WebOSPlayer::UNLOADED) ? 0 : 500;
  if (dispose_timeout_source_) {
    g_source_destroy(dispose_timeout_source_);
  }
  dispose_timeout_source_ = WebOSPlayer::attachToGMainLoop([](void* data) -> int {
    LogError(TAG) << "Dispose timeout.. forced_dispose..";
    AudioPlayer* self = (AudioPlayer*)data;
    self->dispose_timeout_source_ = nullptr;
    self->disposed_cb_(self->player_id_);
    return G_SOURCE_REMOVE;
  }, this, timeout);

  Release();
}

int64_t AudioPlayer::GetDuration() {
  return player_->getDuration();
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

void AudioPlayer::SetVisibility(bool visibility) {
  player_->notifyVisibility(visibility);
}

void AudioPlayer::OnStateChanged(int state) {
  if (disposed_cb_) {
    if (state == WebOSPlayer::UNLOADED) {
      LogInfo(TAG) << cut_url(uri_) << " media pipeline unloaded !!";
      if (dispose_timeout_source_) {
        g_source_destroy(dispose_timeout_source_);
        dispose_timeout_source_ = nullptr;
      }
      disposed_cb_(player_id_);
    }
    return;
  }

  LogInfo(TAG) << "OnStateChanged("
                   << debug_id_
                   << "," << player_->stateToString(state) << ")";
  switch (state) {
    case WebOSPlayer::LOADED:
      reload_ = false;
      if (request_play_) {
        player_->play();
      }
      break;

    case WebOSPlayer::REMOVED:
      removed_ = true;
      reload_ = false;
      seek_position_ = player_->getCurrentPosition();
      if (request_play_) {
        request_play_ = false;
        sink_player_state(PAUSE);
      }
      player_->unload();
      break;

    default:
      break;
  }
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

    int64_t duration = player_->getDuration();
    LogInfo(TAG) << "OnInitialized(" << debug_id_
                 << ",duration:" << duration << ")";

    flutter::EncodableMap map = {{flutter::EncodableValue("event"),
                                  flutter::EncodableValue("audio.onDuration")},
                                 {flutter::EncodableValue("value"),
                                  flutter::EncodableValue(duration)}};
    PushEvent(flutter::EncodableValue(map));
  }
}

void AudioPlayer::OnPlayCompleted() {
  switch (release_mode_) {
    case ReleaseMode::kLoop:
      if (!removed_ && !reload_) {
        player_->seek(0, true);
        if (request_play_) {
          Play();
        }
      }
      break;

    case ReleaseMode::kStop:
      sink_play_completed();
      Stop();
      break;

    case ReleaseMode::kRelease:
      sink_play_completed();
      Release(false);
      break;

    default:
      break;
  }
}

void AudioPlayer::OnSeekCompleted() {
}

void AudioPlayer::OnBufferingStart() {
}

void AudioPlayer::OnBufferingEnd() {
}

void AudioPlayer::OnBuffered(int64_t start, int64_t end) {
}

void AudioPlayer::OnError(int64_t code, const std::string& text) {
  LogError(TAG) << "OnError(" << debug_id_
      << "," << code
      << "," << text << ")";

  sink_player_state(STOP);
  Release(false);

  if (event_sink_ == nullptr) {
    pending_error_code_ = code;
    pending_error_text_ = text;
    return;
  }
  pending_error_code_ = 0;
  pending_error_text_.clear();

  event_sink_->Error(std::to_string(code), text);
}

void AudioPlayer::removeDataSource() {
  std::string path = "/tmp/";
  path += app_id_;
  path += "/";
  path += player_id_;
  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }
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
