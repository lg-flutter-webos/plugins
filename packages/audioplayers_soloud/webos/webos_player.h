// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef WEBOS_PLAYER_
#define WEBOS_PLAYER_

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include <glib.h>

class PluginPlayerDelegate;

// Thin wrapper around the platform media playback API. This plugin only
// ever instantiates a single WebOSPlayer (held by DummyPlayer to keep a
// background audio route alive) so no slot pool or eviction logic is
// needed. All native calls are serialised by a static mutex so that
// selecting the player and issuing the call are atomic across threads.
class WebOSPlayer {
 public:
  static const int UNLOADED = 0;
  static const int IN_LOADING = 1;
  static const int LOADED = 2;
  static const int PLAYING = 3;
  static const int PAUSED = 4;
  static const int EOS = 5;
  static const int REMOVED = 6;

  explicit WebOSPlayer(PluginPlayerDelegate* delegate);
  ~WebOSPlayer();

  WebOSPlayer(const WebOSPlayer&) = delete;
  WebOSPlayer& operator=(const WebOSPlayer&) = delete;

  // hint can carry transport-type clues (e.g. "MPEG-DASH", "HLS",
  // "lowDelayMode"). Pass an empty string when not applicable.
  bool load(const std::string& appId,
            const std::string& winId,
            const std::string& uri,
            const std::string& hint = "",
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
    std::string hint;
    int64_t start_position;
  };

  // Single trampoline registered with the platform event subscription.
  // The platform invokes this on its own thread; it queues the JSON
  // payload for processing on the main GMainLoop.
  static void OnSlotEvent(char* json);

  // Called on the platform's event thread; copies the payload onto the
  // queue and attaches an idle source to drain it on the main loop.
  void EnqueueRawEvent(const char* json);
  void DrainEventQueue();
  void HandleEvent(const std::string& json);
  void HandleErrorEvent(int64_t errorCode);

  std::string BuildLoadPayload(const std::string& appId,
                               const std::string& winId,
                               const std::string& uri,
                               const std::string& hint,
                               int64_t start_position);

  // Serialises the player-selection + call sequence against other threads.
  static std::mutex& MediaCallMutex();
  // The single active instance, used by the static trampoline to route
  // platform events back. Protected by MediaCallMutex().
  static WebOSPlayer* active_instance_;
  // Slot 0 is init'd at most once for the process lifetime. We never call
  // the corresponding Quit because doing so empirically unregisters a
  // global app-state hook with cross-slot side effects on other plugins
  // sharing the same process.
  static bool slot_initialized_;

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
