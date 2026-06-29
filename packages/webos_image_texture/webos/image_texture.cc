// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: LicenseRef-LGE-Proprietary

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>

#include <flutter/event_stream_handler_functions.h>
#include <flutter/standard_method_codec.h>

#include "image_provider.h"
#include "image_texture.h"
#include "log.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "ImageTexture::"

static std::recursive_mutex &render_mutex_ = *new std::recursive_mutex;

/**********************************************************/
/*                  Texture                               */
/**********************************************************/
flutter::TextureRegistrar *Texture::texture_registrar_ = nullptr;
std::list<std::unique_ptr<Texture>> Texture::textures_;

Texture::Texture(flutter::TextureRegistrar *texture_registrar) {
  if (texture_registrar_ == nullptr) {
    texture_registrar_ = texture_registrar;
  }

  upload_pbuffer_ = std::make_unique<FlutterDesktopPixelBuffer>();
  std::memset(upload_pbuffer_.get(), 0, sizeof(FlutterDesktopPixelBuffer));
  upload_pbuffer_->length = 4;
  upload_pbuffer_->glFormat = GL_RGBA;

  pbuffer_ = std::make_unique<FlutterDesktopPixelBuffer>();
  std::memset(pbuffer_.get(), 0, sizeof(FlutterDesktopPixelBuffer));
  pbuffer_->length = 4;
  pbuffer_->glFormat = GL_RGBA;

  texture_variant_ = std::make_unique<flutter::TextureVariant>(
      flutter::PixelBufferTexture([this](size_t width, size_t height)
          -> const FlutterDesktopPixelBuffer * {
        std::lock_guard<std::recursive_mutex> autoLock(render_mutex_);
        if (!pbuffer_ || pbuffer_->buffer == nullptr || pbuffer_->length == 0) {
          return nullptr;
        }
        upload_pbuffer_->buffer = pbuffer_->buffer;
        upload_pbuffer_->width = pbuffer_->width;
        upload_pbuffer_->height = pbuffer_->height;
        upload_pbuffer_->release_context = pbuffer_->release_context;
        upload_pbuffer_->release_callback = pbuffer_->release_callback;
        upload_pbuffer_->length = pbuffer_->length;
        upload_pbuffer_->glFormat = pbuffer_->glFormat;
        return upload_pbuffer_.get();
      }));

  texture_id_ = texture_registrar_->RegisterTexture(texture_variant_.get());
}

Texture::~Texture() {
  if (texture_registrar_ && texture_id_ != -1) {
    texture_registrar_->UnregisterTexture(texture_id_);
  }
}

Texture *Texture::get_texture(flutter::TextureRegistrar *texture_registrar) {
  textures_.remove_if([](const std::unique_ptr<Texture> &t) {
    return t->release_;
  });

  auto texture = std::make_unique<Texture>(texture_registrar);
  Texture *p = texture.get();
  textures_.push_back(std::move(texture));
  return p;
}

// widget.visibility: true (glTexImage2D)
void Texture::activate(const uint8_t *buffer, size_t width, size_t height,
                       void (*release_callback)(void *release_context),
                       void *release_context, size_t length,
                       uint32_t glFormat) {
  std::lock_guard<std::recursive_mutex> autoLock(render_mutex_);
  pbuffer_->buffer = buffer;
  pbuffer_->width = width;
  pbuffer_->height = height;
  pbuffer_->release_context = release_context;
  pbuffer_->release_callback = release_callback;
  pbuffer_->length = length;
  pbuffer_->glFormat = glFormat;
}

// widget.visibility: false (glDetextTexture)
void Texture::deactivate() {
  {
    std::lock_guard<std::recursive_mutex> autoLock(render_mutex_);
    pbuffer_->buffer = nullptr;
    pbuffer_->length = 0;
    pbuffer_->release_context = nullptr;
    pbuffer_->release_callback = nullptr;

    upload_pbuffer_->buffer = nullptr;
    upload_pbuffer_->length = 0;
    upload_pbuffer_->release_context = nullptr;
    upload_pbuffer_->release_callback = nullptr;
  }
  Mark();
}

// do nothing
void Texture::stop() {
  std::lock_guard<std::recursive_mutex> autoLock(render_mutex_);
  pbuffer_->buffer = nullptr;
  pbuffer_->release_context = nullptr;
  pbuffer_->release_callback = nullptr;

  upload_pbuffer_->buffer = nullptr;
  upload_pbuffer_->release_context = nullptr;
  upload_pbuffer_->release_callback = nullptr;
}

// widget.dispose()
void Texture::release() {
  deactivate();
  if (texture_registrar_) {
    texture_registrar_->UnregisterTexture(texture_id_);
    texture_id_ = -1;
  }
  release_ = true;
}

void Texture::release_all() {
  texture_registrar_ = nullptr;
}

/**********************************************************/
/*                  ImageTexture                          */
/**********************************************************/

ImageTexture::ImageTexture(flutter::PluginRegistrar *plugin_registrar,
                           flutter::TextureRegistrar *texture_registrar,
                           bool visibility)
    : visibility_(visibility), provider_(this) {
  g_main_context_ = g_main_context_ref_thread_default();
  texture_ = Texture::get_texture(texture_registrar);
  texture_id_ = texture_->texture_id();
  SetUpEventChannel(plugin_registrar->messenger());
}

ImageTexture::~ImageTexture() {
  exitThread();

  sem_mutex_.lock();
  if (!about_disposed_) {
    exit_sem_ = std::make_unique<Semaphore>();
    sem_mutex_.unlock();

    LogInfo(TAG) << "exit_sem pend";
    exit_sem_->pend();
  } else {
    sem_mutex_.unlock();
  }

  is_initialized_ = false;
  event_sink_ = nullptr;
  event_channel_->SetStreamHandler(nullptr);

  if (idle_source_) {
    g_source_destroy(idle_source_);
    idle_source_ = nullptr;
  }
  g_main_context_unref(g_main_context_);

  texture_->set_deferred_buffer(std::move(texture_buf_));
  texture_->release();
}

void ImageTexture::Play() {
  stop_ = false;
  play_ = true;
  if (visibility_) {
    provider_.Event();
  }
}

void ImageTexture::Stop() {
  play_ = false;
  stop_ = true;
  provider_.Event();
}

void ImageTexture::Visibility(bool visibility) {
  visibility_ = visibility;
  if (visibility && play_) {
    provider_.Event();
  }
}

void ImageTexture::Dispose(const DisposedCallback disposed_cb) {
  disposed_cb_ = disposed_cb;
  deactivated_ = false;
  if (ImageTextureThread::GetInstance().GetHttpEvent().erase(this)) {
    about_disposed_ = true;
    disposed_ = true;
  } else if (ImageTextureThread::GetInstance().GetFileEvent().erase(this)) {
    about_disposed_ = true;
    disposed_ = true;
  }

  if (disposed_ && disposed_cb_) {
    auto cb = std::move(disposed_cb_);
    cb(texture_id_);
  } else {
    exitThread();
  }
}

void ImageTexture::Activate() {
  if (deactivated_) {
    deactivated_ = false;

    run_ = true;
    disposed_cb_ = nullptr;
    about_disposed_ = false;
    disposed_ = false;
    is_initialized_ = false;
    width_ = 0;
    height_ = 0;
    mark_done_ = false;
    Decode();
  }
}

void ImageTexture::Deactivate() {
  if (url_.empty() && in_data_.empty()) {
    return;
  }
  if (disposed_cb_ || !is_initialized_) {
    return;
  }

  exitThread();
  texture_->deactivate();
  texture_->set_deferred_buffer(std::move(texture_buf_));
  texture_buf_size_ = 0;
  deactivated_ = true;
}

void ImageTexture::GetSize(
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (!is_initialized_) {
    pending_size_results_.push_back(std::move(result));
  } else {
    std::vector<std::int64_t> list;
    list.push_back(width_);
    list.push_back(height_);
    result->Success(flutter::EncodableValue(list));
  }
}

void ImageTexture::SendPendingSize() {
  for (auto &result : pending_size_results_) {
    std::vector<std::int64_t> list;
    list.push_back(width_);
    list.push_back(height_);
    result->Success(flutter::EncodableValue(list));
  }
  pending_size_results_.clear();
}

void ImageTexture::SetUrl(const std::string &url, int cacheWidth,
                          int cacheHeight) {
  if (!url_.empty()) {
    if (url_ != url) {
      LogError(TAG) << "Skip SetUrl(" << url << ")";
    }
    return;
  }

  url_ = url;
  cache_width_ = cacheWidth;
  cache_height_ = cacheHeight;

  run_ = true;
  disposed_cb_ = nullptr;
  about_disposed_ = false;
  disposed_ = false;
  Decode();
}

void ImageTexture::SetDataSource(std::vector<uint8_t> &data, int cacheWidth,
                                 int cacheHeight) {
  if (!in_data_.empty()) {
    if (in_data_.data() != data.data()) {
      LogError(TAG) << "Skip SetDataSource()";
    }
    return;
  }
  LogDebug(TAG) << "SetDataSource(" << static_cast<void *>(data.data()) << ") "
                << texture_id_;
  in_data_.swap(data);
  cache_width_ = cacheWidth;
  cache_height_ = cacheHeight;

  run_ = true;
  disposed_cb_ = nullptr;
  about_disposed_ = false;
  disposed_ = false;
  Decode();
}

void ImageTexture::Decode() {
  if (!url_.empty()) {
    if (url_.find("http") == 0) {
      ImageTextureThread::GetInstance().GetHttpEvent().post(this);
    } else {
      ImageTextureThread::GetInstance().GetFileEvent().post(this);
    }
  } else if (!in_data_.empty()) {
    ImageTextureThread::GetInstance().GetFileEvent().post(this);
  }
}

void ImageTexture::SetUpEventChannel(flutter::BinaryMessenger *messenger) {
  std::string name = "webos/image_texture/event" + std::to_string(texture_id_);
  auto channel =
      std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
          messenger, name, &flutter::StandardMethodCodec::GetInstance());
  //  initialized event will be send in listen function of event channel
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

/**********************************************************/
/*                  ImageTextureThread                    */
/**********************************************************/

ImageTextureThread::~ImageTextureThread() { Stop(); }

void ImageTextureThread::httpThread() {
  while (true) {
    ImageTexture *texture = http_event_.pend();
    if (texture == nullptr) {
      break;
    }
    if (!texture->url_.empty()) {
      texture->provider_.Decode(texture->url_, texture->cache_width_,
                                texture->cache_height_);
    } else {
      texture->provider_.Decode(texture->in_data_.data(),
                                texture->in_data_.size(), texture->cache_width_,
                                texture->cache_height_);
      texture->in_data_.clear();
    }
    texture->OnDisposed();
  }
}

void ImageTextureThread::fileThread() {
  while (true) {
    ImageTexture *texture = file_event_.pend();
    if (texture == nullptr) {
      break;
    }
    if (!texture->url_.empty()) {
      texture->provider_.Decode(texture->url_, texture->cache_width_,
                                texture->cache_height_);
    } else {
      texture->provider_.Decode(texture->in_data_.data(),
                                texture->in_data_.size(), texture->cache_width_,
                                texture->cache_height_);
      texture->in_data_.clear();
    }
    texture->OnDisposed();
  }
}

void ImageTextureThread::Start() {
  if (started_) {
    return;
  }
  started_ = true;

  // HTTP Thread Pool
  for (int i = 0; i < MAX_HTTP_THREAD_NUM; i++) {
    http_threads_[i] = new std::thread(&ImageTextureThread::httpThread, this);
  }

  // File Thread Pool
  for (int i = 0; i < MAX_FILE_THREAD_NUM; i++) {
    file_threads_[i] = new std::thread(&ImageTextureThread::fileThread, this);
  }
}

void ImageTextureThread::cleanThread() {
  if (!threads_exited_) {
    threads_exited_ = true;

    for (int i = 0; i < MAX_HTTP_THREAD_NUM; ++i) {
      http_event_.post(nullptr);
    }

    for (int i = 0; i < MAX_FILE_THREAD_NUM; ++i) {
      file_event_.post(nullptr);
    }

    for (int i = 0; i < MAX_HTTP_THREAD_NUM; ++i) {
      if (http_threads_[i]) {
        if (http_threads_[i]->joinable()) {
          http_threads_[i]->join();
        }
        delete http_threads_[i];
        http_threads_[i] = nullptr;
      }
    }

    for (int i = 0; i < MAX_FILE_THREAD_NUM; ++i) {
      if (file_threads_[i]) {
        if (file_threads_[i]->joinable()) {
          file_threads_[i]->join();
        }
        delete file_threads_[i];
        file_threads_[i] = nullptr;
      }
    }
  }
}

void ImageTextureThread::Stop() {
  if (!started_) {
    return;
  }
  cleanThread();
  started_ = false;
}

////////////////////////////////////////////////////////////////
// Player Callbacks
////////////////////////////////////////////////////////////////
void ImageTexture::OnInitialized(uint8_t *buf, uint32_t width, uint32_t height,
                                 uint32_t glFormat, uint32_t length,
                                 bool animation) {
  if (!run_) {
    return;
  }

  texture_buf_ = std::make_unique<uint8_t[]>(length);
  texture_buf_size_ = length;
  uint8_t *dst = texture_buf_.get();
  ::memcpy(dst, buf, length);
  if (!animation) {
    provider_.OnUploaded();
  }

  InitializedCb cb = {width, height, dst, length, glFormat, animation};
  pushCallback(cb);
}

void ImageTexture::execInitialized(size_t width, size_t height, uint8_t *buffer,
                                   size_t length, uint32_t glFormat,
                                   bool animation) {
  if (!run_) {
    return;
  }

  is_initialized_ = true;
  width_ = width;
  height_ = height;
  animation_ = animation;
  SendPendingSize();

  flutter::EncodableMap result = {
      {flutter::EncodableValue("event"),
       flutter::EncodableValue("initialized")},
      {flutter::EncodableValue("width"), flutter::EncodableValue(width_)},
      {flutter::EncodableValue("height"), flutter::EncodableValue(height_)},
  };
  PushEvent(flutter::EncodableValue(result));

  texture_->activate(
      buffer, width, height,
      [](void *release_context) -> void {
        ImageTexture *self = (ImageTexture *)release_context;
        if (self && self->run_ && !self->mark_done_) {
          BasicCb cb = {kTypeUploaded};
          self->pushCallback(cb);
        }
      },
      this, length, glFormat);
  texture_->Mark();
}

void ImageTexture::execUploaded() {
  if (run_ && !mark_done_) {
    mark_done_ = true;
    if (!animation_) {
      run_ = false;
      texture_->stop();
    }
  }
}

void ImageTexture::OnFrame(uint8_t *buf, size_t length) {
  if (!run_ || !buf || length == 0) {
    LogWarn(TAG) << "OnFrame skipped: invalid state (run=" << run_ << ", buf=" << (buf ? "valid" : "null") << ", length=" << length << ")";
    return;
  }
  {
    std::lock_guard<std::recursive_mutex> autoLock(render_mutex_);
    if (texture_buf_ && length <= texture_buf_size_) {
      uint8_t *dst = texture_buf_.get();
      ::memcpy(dst, buf, length);
    }
  }
  BasicCb cb = {kTypeFrame};
  pushCallback(cb);
}

void ImageTexture::execFrame() {
  if (run_) {
    texture_->Mark();

    flutter::EncodableMap result = {
        {flutter::EncodableValue("event"), flutter::EncodableValue("frame")}};
    PushEvent(flutter::EncodableValue(result));
  }
}

void ImageTexture::OnEOS() {
  if (run_) {
    LogDebug(TAG) << "OnEOS(" << url_ << ")";

    BasicCb cb = {kTypeEOS};
    pushCallback(cb);
  }
}

void ImageTexture::execEOS() {
  if (run_) {
    flutter::EncodableMap result = {{flutter::EncodableValue("event"),
                                     flutter::EncodableValue("completed")}};
    PushEvent(flutter::EncodableValue(result));

    run_ = false;
    texture_->stop();
  }
}

void ImageTexture::OnError(int errCode, const std::string &errText) {
  if (run_) {
    LogWarn(TAG) << "OnError(" << url_ << ")(" << errCode << "," << errText
                 << ")";

    ErrorCb cb = {errCode, errText};
    pushCallback(cb);
  }
}

void ImageTexture::execError(int code, const std::string &text) {
  if (!is_initialized_) {
    SendPendingSize();
  }

  if (run_) {
    if (event_sink_ == nullptr) {
      pending_error_code_ = code;
      pending_error_text_ = text;
      return;
    }

    exitThread();
    is_initialized_ = true;
    pending_error_code_ = 0;
    pending_error_text_.clear();
    flutter::EncodableMap result = {
        {flutter::EncodableValue("event"), flutter::EncodableValue("error")},
        {flutter::EncodableValue("errorMsg"), flutter::EncodableValue(text)},
    };
    event_sink_->Success(flutter::EncodableValue(result));
  }
}

void ImageTexture::OnDisposed() {
  BasicCb cb = {kTypeDisposed};
  pushCallback(cb);

  Semaphore *sem = nullptr;
  {
    std::lock_guard<std::recursive_mutex> autoLock(sem_mutex_);
    about_disposed_ = true;
    if (exit_sem_) {
      sem = exit_sem_.get();
    }
  }

  if (sem) {
    LogInfo(TAG) << "exit_sem post";
    sem->post();
  }
}

void ImageTexture::execDisposed() {
  disposed_ = true;
  if (disposed_cb_) {
    auto cb = std::move(disposed_cb_);
    cb(texture_id_);
  }
}

void ImageTexture::pushCallback(CallbackVariant cb) {
  std::lock_guard<std::recursive_mutex> autoLock(cb_mutex_);
  callbacks_.push_back(cb);

  if (idle_source_ == nullptr) {
    idle_source_ = attachToGMainLoop(
        [](void *data) -> int {
          ImageTexture *self = (ImageTexture *)data;
          self->popCallbacks();
          return G_SOURCE_REMOVE;
        },
        this);
  }
}

void ImageTexture::popCallbacks() {
  std::vector<CallbackVariant> callbacks;
  {
    std::lock_guard<std::recursive_mutex> autoLock(cb_mutex_);
    idle_source_ = nullptr;
    std::swap(callbacks_, callbacks);
  }

  for (auto &cb : callbacks) {
    if (auto basic_cb = std::get_if<BasicCb>(&cb)) {
      switch (basic_cb->type) {
      case kTypeUploaded:
        execUploaded();
        break;
      case kTypeFrame:
        execFrame();
        break;
      case kTypeEOS:
        execEOS();
        break;
      case kTypeDisposed:
        LogDebug(TAG) << "kTypeDisposed, skipping remaining callbacks";
        execDisposed();
        return;
      default:
        break;
      }
    } else if (auto icb = std::get_if<InitializedCb>(&cb)) {
      execInitialized(icb->width, icb->height, icb->buffer, icb->length,
                      icb->glFormat, icb->animation);
    } else if (auto error_cb = std::get_if<ErrorCb>(&cb)) {
      execError(error_cb->code, error_cb->text);
    }
  }
}

void ImageTexture::PushEvent(const flutter::EncodableValue &value) {
  if (event_sink_) {
    event_sink_->Success(value);
  } else {
    pending_events_.push_back(value);
  }
}

void ImageTexture::FlushPendingEvents() {
  if (event_sink_) {
    for (auto &value : pending_events_) {
      event_sink_->Success(value);
    }
    pending_events_.clear();

    if (pending_error_code_) {
      execError(pending_error_code_, std::string(pending_error_text_));
    }
  }
}

////////////////////////////////////////////////////////////////
// GMainLoop
////////////////////////////////////////////////////////////////
GSource *ImageTexture::attachToGMainLoop(GMainLoopCallback cb, void *data) {
  GSource *src = g_idle_source_new();
  g_source_set_callback(src, cb, data, NULL);
  g_source_attach(src, g_main_context_);
  g_source_unref(src);
  return src;
}

void ImageTexture::exitThread() {
  if (run_) {
    run_ = false;
    texture_->stop();
    provider_.Event();
  }
}
