// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef AUDIO_PLAYER_H_
#define AUDIO_PLAYER_H_

#include <flutter/encodable_value.h>
#include <flutter/event_channel.h>
#include <flutter/plugin_registrar.h>
#include "plugin_player_delegate.h"
#include "webos_player.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <glib.h>

enum class ReleaseMode { kRelease, kLoop, kStop };

class AudioPlayer : public PluginPlayerDelegate {
 public:
  static const int64_t ERROR_PLUGIN = -1;
  using DisposedCallback = std::function<void(const std::string&)>;

  AudioPlayer(flutter::PluginRegistrar* plugin_registrar,
              const std::string &player_id, int debug_id,
              const bool visibility);
  virtual ~AudioPlayer();

  void SetUrl(const std::string &url, const std::string& mime_type);
  void SetDataSource(std::vector<uint8_t> &data, const std::string& mime_type);
  std::string GetPlayerId() const { return player_id_; }

  void Play();
  void Pause();
  void SetReleaseMode(ReleaseMode mode);
  void SetPlaybackRate(double playback_rate);
  void Seek(int64_t position);
  int64_t GetPosition();
  void Stop();
  void Release(bool deep = true);
  void Dispose(const DisposedCallback disposed_cb);

  void SetVisibility(bool visibility);
  bool IsPlaying() { return request_play_; }
  int64_t GetDuration();

// Call at WebOSPlayer.
  virtual void OnStateChanged(int state) override;
  virtual void OnInitialized() override;
  virtual void OnPlayCompleted() override;
  virtual void OnSeekCompleted() override;
  virtual void OnBufferingStart() override;
  virtual void OnBufferingEnd() override;
  virtual void OnBuffered(int64_t start, int64_t end) override;
  virtual void OnError(int64_t errCode,const std::string& errText) override;

 private:
  void OnPrepared();

  void SetUpEventChannel(flutter::BinaryMessenger *messenger);
  void PushEvent(const flutter::EncodableValue& value);
  void FlushPendingEvents();

  void loadCommon();
  void sink_seek_completed();
  void sink_play_completed();
  void sink_player_state(const std::string& state);
  std::string releaseModeToString(ReleaseMode mode);
  void removeDataSource();

  const std::string player_id_;
  int debug_id_ = 0;
  std::string app_id_;
  std::string uri_;
  std::string mime_type_;

  bool request_play_ = false;
  ReleaseMode release_mode_ = ReleaseMode::kRelease;
  bool removed_ = false;
  bool reload_ = false;
  int64_t pending_error_code_ = 0;
  std::string pending_error_text_;
  std::unique_ptr<WebOSPlayer> player_;

  bool is_initialized_ = false;
  int64_t seek_position_ = 0;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;

  DisposedCallback disposed_cb_ = nullptr;
  GSource* dispose_timeout_source_ = nullptr;
  std::vector<flutter::EncodableValue> pending_events_;
};
#endif  // FLUTTER_PLUGIN_AUDIO_PLAYER_H_
