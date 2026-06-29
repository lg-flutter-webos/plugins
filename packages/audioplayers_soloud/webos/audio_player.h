// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef AUDIO_PLAYER_H_
#define AUDIO_PLAYER_H_

#include <flutter/encodable_value.h>
#include <flutter/event_channel.h>
#include <flutter/plugin_registrar.h>

#include <functional>
#include <string>
#include <vector>
#include <set>
#include <glib.h>

enum class ReleaseMode { kRelease, kLoop, kStop };

class AudioPlayer {
 public:
  static const int64_t ERROR_PLUGIN = -1;

  AudioPlayer(flutter::PluginRegistrar* plugin_registrar,
              const std::string &player_id, const int debug_id);
  virtual ~AudioPlayer();

  void SetUrl(const std::string &url, const std::string& mime_type);
  void SetDataSource(std::vector<uint8_t> &data, const std::string& mime_type);
  std::string GetPlayerId() const { return player_id_; }

  void Play();
  void Pause();
  void SetReleaseMode(ReleaseMode mode);
  void SetVolume(double volume);
  void SetPlaybackRate(double playback_rate);
  void SetBalance(double balance);
  void Seek(int64_t position);
  int64_t GetPosition();
  void Stop();
  void Release();
  void Dispose(bool deep = false);

  bool IsPlaying() { return request_play_; }
  void SetLatencyMode(bool low_latency);
  int64_t GetDuration();

  void OnInitialized();
  void OnPlayCompleted();
  void OnError(int64_t errCode,const std::string& errText);

 private:
  void OnPrepared();

  typedef int (*GMainLoopCallback)(void*);
  static GSource* attachToGMainLoop(GMainLoopCallback cb,void* data,int64_t delayed = 0);
  void getHttpCommon();
  void onHttp();

  void SetUpEventChannel(flutter::BinaryMessenger *messenger);
  void PushEvent(const flutter::EncodableValue& value);
  void FlushPendingEvents();
  void loadMem(uint8_t* data, size_t size);
  void loadFile(const std::string& file_name);
  void loadCommon();
  int64_t getCurrentPosition();
  void sink_seek_completed();
  void sink_play_completed();
  void sink_player_state(const std::string& state);
  std::string releaseModeToString(ReleaseMode mode);

  const std::string player_id_;
  int debug_id_ = 0;
  std::string uri_;
  uint8_t* data_source_ = nullptr;
  size_t data_size_ = 0;
  bool low_latency_ = false;
  bool opus_ = false;

  bool request_play_ = false;
  ReleaseMode release_mode_ = ReleaseMode::kRelease;
  int64_t pending_error_code_ = 0;
  std::string pending_error_text_;

  bool is_initialized_ = false;
  int64_t seek_position_ = 0;
  int64_t current_position_ = 0;
  int64_t duration_ = 0;
  double volume_ = 1.0;
  double balance_ = 0.0;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
  std::vector<flutter::EncodableValue> pending_events_;

  unsigned int hash_ = 0;
  unsigned int handle_ = 0;
};
#endif  // FLUTTER_PLUGIN_AUDIO_PLAYER_H_
