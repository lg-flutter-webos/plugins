// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: LicenseRef-LGE-Proprietary

#ifndef IMAGE_TEXTURE_H_
#define IMAGE_TEXTURE_H_

#include <flutter/encodable_value.h>
#include <flutter/event_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/texture_registrar.h>

#include "image_provider.h"
#include <glib.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#define MAX_FILE_THREAD_NUM 4
#define MAX_HTTP_THREAD_NUM 4

class ImageTexture;

class Semaphore {
public:
  Semaphore(int count = 0) : count_(count) {}

  void pend() {
    std::unique_lock<std::mutex> lck(mutex_);
    cv_.wait(lck, [this]() { return count_ != 0; });
    count_--;
  }

  void post() {
    std::lock_guard<std::mutex> lck(mutex_);
    count_++;
    cv_.notify_one();
  }

  bool try_pend() {
    std::lock_guard<std::mutex> lck(mutex_);
    if (count_ == 0) {
      return false;
    }
    count_--;
    return true;
  }

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  unsigned int count_;
};

class Event {
public:
  Event(int count = 0) : count_(count) {}

  ImageTexture *pend() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]() { return count_ != 0; });
    count_--;

    ImageTexture *texture = nullptr;
    if (!textures_.empty()) {
      texture = textures_.front();
      textures_.pop_front();
    }
    return texture;
  }

  void post(ImageTexture *texture) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (texture) {
      for (auto it = textures_.begin(); it != textures_.end(); ++it) {
        if (*it == texture) {
          return;
        }
      }
    }
    count_++;
    textures_.push_back(texture);
    cv_.notify_one();
  }

  bool erase(ImageTexture *texture) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = textures_.begin(); it != textures_.end(); ++it) {
      if (*it == texture) {
        textures_.erase(it);
        count_--;
        return true;
      }
    }
    return false;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    textures_.clear();
  }

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  unsigned int count_;
  std::list<ImageTexture *> textures_;
};

class Texture {
public:
  Texture(flutter::TextureRegistrar *texture_registrar);
  virtual ~Texture();

  static Texture *get_texture(flutter::TextureRegistrar *texture_registrar);
  static void release_all();
  void activate(const uint8_t *buffer, size_t width, size_t height,
                void (*release_callback)(void *release_context),
                void *release_context, size_t length, uint32_t glFormat);
  void deactivate();
  void stop();
  void release();
  void set_deferred_buffer(std::unique_ptr<uint8_t[]> buf) {
    deferred_buffer_ = std::move(buf);
  }
  int64_t texture_id() { return texture_id_; }
  void Mark() {
    if (texture_registrar_) {
      texture_registrar_->MarkTextureFrameAvailable(texture_id_);
    }
  }

private:
  std::unique_ptr<flutter::TextureVariant> texture_variant_;
  std::unique_ptr<FlutterDesktopPixelBuffer> pbuffer_;
  std::unique_ptr<FlutterDesktopPixelBuffer> upload_pbuffer_;

  bool release_ = false;
  int64_t texture_id_ = -1;
  std::unique_ptr<uint8_t[]> deferred_buffer_;

  static flutter::TextureRegistrar *texture_registrar_;
  static std::list<std::unique_ptr<Texture>> textures_;
};

class ImageTexture {
public:
  using DisposedCallback = std::function<void(int64_t)>;

  ImageTexture(flutter::PluginRegistrar *plugin_registrar,
               flutter::TextureRegistrar *texture_registrar, bool visibility);
  virtual ~ImageTexture();
  int64_t GetTextureId() { return texture_id_; }

  void SetUrl(const std::string &url, int cacheWidth, int cacheHeight);
  void SetDataSource(std::vector<uint8_t> &data, int cacheWidth,
                     int cacheHeight);
  void Play();
  void Stop();
  void Visibility(bool visibility);
  void Dispose(const DisposedCallback disposed_cb);
  void Activate();
  void Deactivate();
  void GetSize(
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  void OnInitialized(uint8_t *buf, uint32_t width, uint32_t height,
                     uint32_t glFormat, uint32_t length, bool animation);
  void OnError(int code, const std::string &text);
  void OnFrame(uint8_t *buf, size_t length);
  void OnEOS();
  void OnDisposed();

  bool IsRun() { return run_; }
  bool IsPlay() { return play_; }
  bool IsStop() { return !play_ && stop_; }

  std::string url_;
  int cache_width_ = 0;
  int cache_height_ = 0;
  std::vector<uint8_t> in_data_;
  ImageProvider provider_;

private:
  enum BasicCbType { kTypeUploaded, kTypeFrame, kTypeEOS, kTypeDisposed };

  typedef struct {
    enum BasicCbType type;
  } BasicCb;

  typedef struct {
    size_t width;
    size_t height;
    uint8_t *buffer;
    size_t length;
    uint32_t glFormat;
    bool animation;
  } InitializedCb;

  typedef struct {
    int code;
    std::string text;
  } ErrorCb;

  typedef std::variant<BasicCb, InitializedCb, ErrorCb> CallbackVariant;

  typedef int (*GMainLoopCallback)(void *);

  void SetUpEventChannel(flutter::BinaryMessenger *messenger);
  void Decode();
  void SendPendingSize();
  void execInitialized(size_t width, size_t height, uint8_t *buffer,
                       size_t length, uint32_t glFormat, bool animation);
  void execUploaded();
  void execFrame();
  void execEOS();
  void execError(int code, const std::string &text);
  void execDisposed();

  void exitThread();
  void pushCallback(CallbackVariant cb);
  void popCallbacks();
  void PushEvent(const flutter::EncodableValue &value);
  void FlushPendingEvents();
  GSource *attachToGMainLoop(GMainLoopCallback cb, void *data);

  bool visibility_ = false;
  std::atomic<bool> run_{false};
  std::atomic<bool> play_{false};
  std::atomic<bool> stop_{false};
  int64_t texture_id_ = -1;
  DisposedCallback disposed_cb_ = nullptr;

  std::atomic<bool> mark_done_{false};
  std::atomic<bool> about_disposed_{false};
  int width_ = 0;
  int height_ = 0;
  bool animation_ = false;
  bool is_initialized_ = false;
  bool deactivated_ = false;
  bool disposed_ = false;
  std::recursive_mutex sem_mutex_;
  std::unique_ptr<uint8_t[]> texture_buf_;
  size_t texture_buf_size_ = 0;
  std::unique_ptr<Semaphore> exit_sem_;
  std::vector<std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>>>
      pending_size_results_;
  std::vector<flutter::EncodableValue> pending_events_;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;

  Texture *texture_ = nullptr;

  std::recursive_mutex cb_mutex_;
  std::vector<CallbackVariant> callbacks_;
  int pending_error_code_ = 0;
  std::string pending_error_text_;
  GSource *idle_source_ = nullptr;
  GMainContext *g_main_context_ = nullptr;
};

class ImageTextureThread {
public:
  static ImageTextureThread &GetInstance() {
    static ImageTextureThread instance;
    return instance;
  }

  ImageTextureThread(const ImageTextureThread &) = delete;
  ImageTextureThread &operator=(const ImageTextureThread &) = delete;

  void Start();
  void Stop();
  Event &GetHttpEvent() { return http_event_; }
  Event &GetFileEvent() { return file_event_; }

private:
  ImageTextureThread() = default;
  ~ImageTextureThread();

  void httpThread();
  void fileThread();
  void cleanThread();

  std::thread *http_threads_[MAX_HTTP_THREAD_NUM]{};
  std::thread *file_threads_[MAX_FILE_THREAD_NUM]{};
  Event http_event_;
  Event file_event_;
  bool started_ = false;
  bool threads_exited_ = false;
};
#endif
