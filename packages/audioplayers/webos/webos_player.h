// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef WEBOS_PLAYER_
#define WEBOS_PLAYER_

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <glib.h>

class PluginPlayerDelegate;

// Thin wrapper around the webOS media playback API. A WebOSPlayer is bound
// to one of the platform's player slots; Create() returns nullptr when
// every slot is in use. All native calls are serialised by a global mutex
// so that selecting the active slot and issuing the call are atomic
// across threads.
class WebOSPlayer {
 public:
  static constexpr int kMaxSlots = 5;

  static const int UNLOADED = 0;
  static const int IN_LOADING = 1;
  static const int LOADED = 2;
  static const int PLAYING = 3;
  static const int PAUSED = 4;
  static const int EOS = 5;
  static const int REMOVED = 6;

  // Returns nullptr if every player slot is in use.
  static std::unique_ptr<WebOSPlayer> Create(PluginPlayerDelegate* delegate);
  ~WebOSPlayer();

  WebOSPlayer(const WebOSPlayer&) = delete;
  WebOSPlayer& operator=(const WebOSPlayer&) = delete;

  bool load(const std::string& appId,
            const std::string& winId,
            const std::string& uri,
            const std::string& webos_payload = "",
            const std::string& webos_format = "",
            int64_t start_position = 0);
  bool unload(bool reset = false);
  bool play();
  bool pause();
  bool seek(int64_t position, bool update_position = false);
  bool setPlayRate(double playRate, bool audioOut);
  bool notifyVisibility(bool visibility);

  int getState() { return state_; }
  int64_t getCurrentPosition();
  int64_t getDuration() { return duration_; }
  bool isSeekable() { return seekable_; }
  bool hasVideo() { return has_video_; }
  bool hasAudio() { return has_audio_; }
  int getVideoWidth() { return videoW_; }
  int getVideoHeight() { return videoH_; }
  std::string stateToString(int state);

  typedef int (*GMainLoopCallback)(void*);
  static GSource* attachToGMainLoop(GMainLoopCallback cb, void* data,
                                    int64_t delayed = 0);

 private:
  enum MediaPipelineStatus {
    SMP_PLAYING_ERROR = 100,
    SMP_COMMAND_NOT_SUPPORTED = 101,
    SMP_NEED_TO_RELOAD_A_PIPELINE = 102,
    SMP_TAKE_SNAPSHOT_ERROR = 103,
    SMP_FAILED_TO_LOAD = 104,
    SMP_AUDIO_CODEC_NOT_SUPPORTED = 200,
    SMP_VIDEO_CODEC_NOT_SUPPORTED = 201,
    SMP_MEDIA_NOT_FOUND = 202,
    SMP_AV_TYPE_NOT_FOUNDED = 203,
    SMP_FAILED_TO_DEMULTIPLEX = 204,
    SMP_UNKNOWN_SUBTITLE = 210,
    SMP_NETWORK_ERROR = 300,
    SMP_UNSECURED_WIFI = 301,
    SMP_HLS_SERVER_ERROR_302 = 400,
    SMP_HLS_SERVER_ERROR_404 = 401,
    SMP_HLS_SERVER_ERROR_503 = 402,
    SMP_HLS_SERVER_ERROR_504 = 403,
    SMP_DRM_RELATED_ERROR = 500,
    SMP_DRM_INVALID_DRM_TYPE_ERROR = 506,
    SMP_RM_RELATED_ERROR = 600,
    SMP_RESOURCE_ALLOCATION_ERROR = 601,
    SMP_DVR_RESOURCE_ALLOCATION_ERROR = 602,
    SMP_SEEK_FAILURE = 700,
    SMP_SETPLAYRATE_FAILURE = 701,
    SMP_PIPELINE_DIED_ABNORMALLY = 1000,
    SMP_STREAMING_PROTOCOL_RELATED_ERROR = 40000,
    SMP_MEDIA_API_PLAYING_ERROR = 61440,
    SMP_BUFFER_FULL = 61441,
    SMP_BUFFER_LOW = 61442,
    SMP_RESOURCE_ACQUIRE_ERROR = 61456,
  };

  struct LoadParams {
    std::string appId;
    std::string winId;
    std::string uri;
    std::string payload;
    std::string format;
    int64_t start_position;
  };

  WebOSPlayer(PluginPlayerDelegate* delegate, int slot);

  // Per-slot trampolines registered with the platform event subscription.
  // The platform invokes these on its own thread; they queue the JSON
  // payload for processing on the main GMainLoop.
  static void OnSlotEvent_0(char* json);
  static void OnSlotEvent_1(char* json);
  static void OnSlotEvent_2(char* json);
  static void OnSlotEvent_3(char* json);
  static void OnSlotEvent_4(char* json);
  static void DispatchToSlot(int slot, char* json);
  static const std::array<void (*)(char*), kMaxSlots> kTrampolines;

  // Called on the platform's event thread; copies the payload onto the
  // queue and attaches an idle source to drain it on the main loop.
  void EnqueueRawEvent(const char* json);
  // Drains the queue on the main loop and dispatches to the delegate.
  void DrainEventQueue();
  // Parses a single JSON event and invokes the matching delegate hook.
  void HandleEvent(const std::string& json);
  void HandleErrorEvent(int64_t errorCode, const std::string& errorText);

  // Helper used by load() to compose the option JSON payload.
  std::string BuildLoadPayload(const std::string& appId,
                               const std::string& winId,
                               const std::string& uri,
                               const std::string& webos_payload,
                               const std::string& webos_format,
                               int64_t start_position);

  // Serialises the slot-selection + call sequence against other threads.
  static std::mutex& MediaCallMutex();
  // Slot ownership table; protected by SlotMutex().
  static std::mutex& SlotMutex();
  static WebOSPlayer* slot_owners_[kMaxSlots];
  // Once a slot is NDL_MediaInit'd we keep it alive for the rest of the
  // process. Calling NDL_MediaQuit during eviction had a cross-slot side
  // effect that invalidated other slots' state (likely via the global
  // libwebos-helper app-state hook deregister), so we never Quit and just
  // clear slot_owners_[i] when a player goes away.
  static bool slots_initialized_[kMaxSlots];

  int slot_;
  PluginPlayerDelegate* delegate_;
  bool visibility_;
  double speed_;
  bool audio_out_;
  int state_;
  std::atomic<int64_t> current_;
  int64_t duration_;
  bool seekable_;
  bool has_video_;
  bool has_audio_;
  int videoW_;
  int videoH_;
  bool buffering_;
  int64_t buffering_start_;
  int64_t buffering_end_;
  LoadParams loadParams_;
  static GMainContext* s_main_context_;

  std::vector<std::string> event_queue_;
  std::recursive_mutex mutex_;
  // Lifecycle guard: cleared in the destructor so that any callback or
  // GSource that fires after teardown becomes a no-op.
  bool run_ = true;
  GSource* idle_source_ = nullptr;
  GSource* timer_source_ = nullptr;
};

#endif  // WEBOS_PLAYER_
