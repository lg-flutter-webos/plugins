// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: LicenseRef-LGE-Proprietary

#include <filesystem>
#include <algorithm>
#include <string>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <curl/curl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "log.h"
#include "image_cache.h"
#include "image_texture.h"
#include "image_provider.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "ImageProvider::"

#define COMPRESS_MIN_DIMENSION (192*192)
#define MAX_LOSSY_COMPRESSION_RATIO 30

static int64_t getCurTick();
static int64_t getDiffTick(int64_t t0);

ImageProvider::ImageProvider(ImageTexture* texture)
 : ImageFactory()
 , texture_(texture) {
}

ImageProvider::~ImageProvider() {
  if (g_loop_) {
    LogError(TAG) << "Destructor critical error";
  }
}

////////////////////////////////////////////////////////////////
// Play/Pause/Run
////////////////////////////////////////////////////////////////
void ImageProvider::Event() {
  if (!texture_->IsRun()) {
    pushEvent(kTypeQuit);
  } else if (texture_->IsPlay()) {
    pushEvent(kTypePlay);
  } else if (texture_->IsStop()) {
    pushEvent(kTypeStop);
  }
}

void ImageProvider::execPlay() {
  if (texture_->IsRun() && animated_ && texture_->IsPlay()) {
    std::lock_guard<std::recursive_mutex> autolock(mutex_);
    if (timer_source_ == nullptr) {
      execAnimationFrame(false);
    }
  }
}

void ImageProvider::execStop() {
  if (texture_->IsRun() && animated_ && texture_->IsStop()) {
    animationCleanUp();
    decodeAnimationNext();
    uint32_t size = (bpp_ == 24) ? 3 : 4;
    texture_->OnFrame(buf_, width_ * height_ * size);
  }
}

void ImageProvider::execAnimationFrame(bool frame) {
  if (texture_->IsRun() && animated_ && texture_->IsPlay()) {
    if (frame) {
      uint32_t size = (bpp_ == 24) ? 3 : 4;
      texture_->OnFrame(buf_, width_ * height_ * size);
    }
    {
      std::lock_guard<std::recursive_mutex> autolock(mutex_);
      timer_source_ = attachToGMainLoop(onAnimationFrame,
                        this,
                        getAnimationDelay());
    }

    bool next = decodeAnimationNext();
    // Last frame
    if (!next) {
      int loop_cnt = getAnimationLoopCount();
      if (loop_cnt != 0 && loop_cnt <= ++animation_cnt_) {
        texture_->OnEOS();
        execQuit();
        return;
      }
      decodeAnimationNext();
    }
  }
}

void ImageProvider::execQuit() {
  std::lock_guard<std::recursive_mutex> autolock(mutex_);
  if (g_loop_) {
    GMainLoop* loop = g_loop_;
    g_loop_ = nullptr;
    g_main_loop_quit(loop);
  }
}

void ImageProvider::execDecodeUrl() {
  if (!decode(url_)) {
    execQuit();
  }
}

void ImageProvider::execDecodeBuf() {
  if (!decode(in_buf_, in_buf_size_)) {
    execQuit();
  }
}

void ImageProvider::execUploaded() {
  if (!animated_) {
    execQuit();
  }
}

////////////////////////////////////////////////////////////////
// OnUploaded
////////////////////////////////////////////////////////////////
void ImageProvider::OnUploaded() {
  pushEvent(kTypeUploaded);
}

////////////////////////////////////////////////////////////////
// Decode
////////////////////////////////////////////////////////////////
#define MAX_MAGIC_BYTES 12

static bool hasCompressedTexture() {
  static bool s_first = true;
  static bool s_compressedTexture = false;
  if (s_first) {
    s_first = false;
    s_compressedTexture = eglGetProcAddress("glCompressedTexImage2D") ? true : false;
    if (!s_compressedTexture) {
      LogWarn(TAG) << "glCompressedTexImage2D was not supported !!!";
    }
  }
  return s_compressedTexture;
}

static bool isASTC(const uint8_t* buf,size_t buf_size) {
  const uint8_t magics[] = {
    0x13, 0xAB, 0xA1, 0x5C,
  };
  static size_t size = sizeof(magics);
  if (buf_size < size) {
    return false;
  }

  for (size_t i = 0; i < size; i++) {
    if (magics[i] != buf[i]) {
      return false;
    }
  }
  return hasCompressedTexture();
}

static bool isPKM(const uint8_t* buf,size_t buf_size) {
  const uint8_t magics[] = {
    0x50, 0x4B, 0x4D,
  };

  static size_t size = sizeof(magics);
  if (buf_size < size) {
    return false;
  }
  for (size_t i = 0; i < size; i++) {
    if (magics[i] != buf[i]) {
      return false;
    }
  }
  return hasCompressedTexture();
}

static bool isKTX(const uint8_t* buf,size_t buf_size) {
  const uint8_t magics[] = {
    0xAB, 0x4B, 0x54, 0x58,
    0x20, 0x31, 0x31, 0xBB,
    0x0D, 0x0A, 0x1A, 0x0A,
  };

  static size_t size = sizeof(magics);
  if (buf_size < size) {
    return false;
  }
  for (size_t i = 0; i < size; i++) {
    if (magics[i] != buf[i]) {
      return false;
    }
  }
  return hasCompressedTexture();
}

static bool isPVR(const uint8_t* buf,size_t buf_size) {
  if (buf_size < sizeof(uint32_t)) {
    return false;
  }

  uint32_t* version = (uint32_t*)buf;
  if (*version != 0x03525650) {
    return false;
  }
  return hasCompressedTexture();
}

static bool isGIF(const uint8_t* buf,size_t buf_size) {
  const uint8_t magics[] = {
    0x47, 0x49, 0x46,
  };

  static size_t size = sizeof(magics);
  if (buf_size < size) {
    return false;
  }
  for (size_t i = 0; i < size; i++) {
    if (magics[i] != buf[i]) {
      return false;
    }
  }
  return true;
}

static bool isPNG(const uint8_t* buf, size_t buf_size) {
  const uint8_t magics[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
  };

  static size_t size = sizeof(magics);
  if (buf_size < size) {
    return false;
  }
  for (size_t i = 0; i < size; i++) {
    if (magics[i] != buf[i]) {
      return false;
    }
  }
  return true;
}

static bool isWebP(const uint8_t* buf, size_t buf_size) {
  const char riff[4] = { 'R', 'I', 'F', 'F' };
  const char webp[4] = { 'W', 'E', 'B', 'P' };
  if (buf_size < 12) {
    return false;
  }

  uint8_t* c0 = (uint8_t*)riff;
  uint8_t* c1 = (uint8_t*)buf;
  for (int i = 0; i < 4; i++) {
    if (*c0++ != *c1++) {
      return false;
    }
  }

  c0 = (uint8_t*)webp;
  c1 = (uint8_t*)buf + 8;
  for (int i = 0; i < 4; i++) {
    if (*c0++ != *c1++) {
      return false;
    }
  }
  return true;
}

static bool isLossyWebP(const uint8_t* buf, size_t buf_size) {
  const char vp8[4] = { 'V', 'P', '8', ' '};
  if (buf_size < 16) {
    return false;
  }

  uint8_t* c0 = (uint8_t*)vp8;
  uint8_t* c1 = (uint8_t*)buf + 12;
  for (int i = 0; i < 4; i++) {
    if (*c0++ != *c1++) {
      return false;
    }
  }
  return true;
}

static bool isJPG(const uint8_t* buf, size_t buf_size) {
  if (buf_size < 3) {
    return false;
  }

  if (buf[0] == 0xff && buf[1] == 0xd8  && buf[2] == 0xff) {
    return true;
  }
  return false;
}

static bool isLossy(const uint8_t* buf, size_t buf_size) {
  return isWebP(buf, buf_size)
      ? isLossyWebP(buf, buf_size)
      : isJPG(buf, buf_size);
}

void ImageProvider::Decode(const std::string& url,
                           int cache_width,int cache_height) {
  GMainContext* context = g_main_context_new();
  GMainLoop* loop = g_main_loop_new(context,FALSE);
  {
    std::lock_guard<std::recursive_mutex> autolock(mutex_);
    events_.clear();
    g_loop_ = loop;
  }

  url_ = url;
  cache_width_ = cache_width;
  cache_height_ = cache_height;
  pushEvent(kTypeDecodeUrl);

  g_main_loop_run(loop);

  {
    std::lock_guard<std::recursive_mutex> autolock(mutex_);
    g_loop_ = nullptr;
    if (idle_source_) {
      g_source_destroy(idle_source_);
      idle_source_ = nullptr;
    }
    if (timer_source_) {
      g_source_destroy(timer_source_);
      timer_source_ = nullptr;
    }
  }

  animated_ = false;
  animation_cnt_ = 0;
  ImageFactory::Dispose();
  httpCleanUp();

  g_main_loop_unref(loop);
  g_main_context_unref(context);
}

void ImageProvider::Decode(uint8_t* buf,size_t buf_size,
                           int cache_width,int cache_height) {
  GMainContext* context = g_main_context_new();
  GMainLoop* loop = g_main_loop_new(context,FALSE);
  {
    std::lock_guard<std::recursive_mutex> autolock(mutex_);
    events_.clear();
    g_loop_ = loop;
  }

  in_buf_ = buf;
  in_buf_size_ = buf_size;
  cache_width_ = cache_width;
  cache_height_ = cache_height;
  pushEvent(kTypeDecodeBuf);

  g_main_loop_run(loop);

  {
    std::lock_guard<std::recursive_mutex> autolock(mutex_);
    g_loop_ = nullptr;
    if (idle_source_) {
      g_source_destroy(idle_source_);
      idle_source_ = nullptr;
    }
    if (timer_source_) {
      g_source_destroy(timer_source_);
      timer_source_ = nullptr;
    }
  }

  animated_ = false;
  animation_cnt_ = 0;
  ImageFactory::Dispose();

  g_main_loop_unref(loop);
  g_main_context_unref(context);
}

bool ImageProvider::decode(const std::string& url,bool re_enter) {
  bool retv = false;
  bool compressed = false;
  bool lossy = false;
  bool resized = false;
  bool isHttp = (url.find("http") == 0) ? true : false;
  std::unique_ptr<uint8_t[]> backup_buf;
  std::string binded_url = ImageCache::bind_url(url,cache_width_,cache_height_);

  // =========================
  // ==== HTTP/File/Asset ====
  // =========================
  httpCleanUp();
  ImageFactory::Dispose();

  std::tie(http_buf_,
           http_buf_size_,
           saved_width_,
           saved_height_) = ImageCache::get_infos(binded_url);
  if (http_buf_) {
    http_no_transform_ = true;
  } else {
    int errCode;
    std::string errText;
    std::tie(errCode,errText) = isHttp ? http(url) : file(url);
    //==== Network error
    if (errCode) {
      texture_->OnError(errCode,errText);
      return false;
    }
  }
  if (!texture_->IsRun()) {
    httpCleanUp();
    return false;
  }

  bool astc = isASTC(http_buf_,http_buf_size_);
  bool pkm = isPKM(http_buf_,http_buf_size_);
  bool ktx = isKTX(http_buf_,http_buf_size_);
  bool pvr = isPVR(http_buf_,http_buf_size_);
  // Compressed Texture
  if (astc || pkm || ktx || pvr) {
    compressed = true;
    if (astc) { retv = decodeASTC(http_buf_,http_buf_size_); }
    else if (pkm) { retv = decodePKM(http_buf_,http_buf_size_); }
    else if (ktx) { retv = decodeKTX(http_buf_,http_buf_size_); }
    else if (pvr) { retv = decodePVR(http_buf_,http_buf_size_); }
  }
  // Animated GIF
  else if (isGIF(http_buf_,http_buf_size_)) {
    retv = decodeAnimationBuf(http_buf_, http_buf_size_, "image/gif");
    animated_ = (getAnimationDelay() > 0) ? true : false;
  }
  // Animated PNG
  else if (isPNG(http_buf_, http_buf_size_) &&
           isAPNG(http_buf_, http_buf_size_)) {
    retv = decodeAnimationBuf(http_buf_, http_buf_size_, "image/apng");
    animated_ = true;
  }
  // Animated WebP
  else if (isWebP(http_buf_, http_buf_size_) &&
           isAnimatedWebP(http_buf_, http_buf_size_)) {
    retv = decodeAnimationBuf(http_buf_, http_buf_size_, "image/webp");
    animated_ = true;
  }
  // JPG,PNG,WEBP,SVG
  else {
    retv = decodeBuf(http_buf_,http_buf_size_,0,cache_width_,cache_height_);
    if (retv && texture_->IsRun()) {
      if (isLossy(http_buf_, http_buf_size_)) {
        size_t compression_ratio = (width_ * height_ * 3) / http_buf_size_;
        if (compression_ratio > MAX_LOSSY_COMPRESSION_RATIO) {
          lossy = true;
        }
      }
      if (!http_no_transform_) {
        resized = fitScaleDown(cache_width_, cache_height_);
      }
      if (bpp_ == 32) {
        if (resized) {
          size_t size = width_ * height_ * 4;
          backup_buf = std::make_unique<uint8_t[]>(size);
          ::memcpy(backup_buf.get(), buf_, size);
        }
        premultiply();
      }
      LogDebug(TAG) << "decode("
                    << url << " -> "
                    << width_ << "x" << height_ << " "
                    << cache_width_ << "x" << cache_height_ << ")";
    }
  }

  // =========================
  // ======= FINALIZE ========
  // =========================
  if (!retv) {
    httpCleanUp();
    ImageFactory::Dispose();
    texture_->OnError(-2,url + " decode fail");
    return false;
  }
  if (!texture_->IsRun()) {
    httpCleanUp();
    ImageFactory::Dispose();
    return false;
  }

  if (!http_can_cache_) {
    if (!compressed) {
      uint32_t glFormat = (bpp_ == 24) ? GL_RGB : GL_RGBA;
      uint32_t size = (bpp_ == 24) ? 3 : 4;
      uint32_t length = width_ * height_ * size;
      texture_->OnInitialized(buf_,width_,height_,glFormat, length, animated_);
    }
    return true;
  }

  bool re_run = false;
  bool insert_needed = true;
  if (compressed || animated_ || http_no_transform_) {
    resized = false;
  } else if (lossy) {
  } else if (resized || width_ * height_ > COMPRESS_MIN_DIMENSION) {
    re_run = ImageCache::insert_compression(binded_url,
                        buf_,width_,height_,bpp_,
                        http_content_length_,http_etag_);
    if (re_run) {
      insert_needed = false;
    }
  }

  if (insert_needed) {
    if (resized) {
      ImageCache::insert_png(binded_url,
              backup_buf ? backup_buf.get() : buf_,
              width_,height_,bpp_,
              http_content_length_,http_etag_);
    } else if (isHttp) {
      ImageCache::insert_raw(binded_url,
              http_buf_,http_buf_size_,width_,height_,
              http_content_length_,http_etag_);
    }
  }

  if (!re_enter && re_run) {
    return decode(url,true);
  }

  if (!compressed) {
    uint32_t glFormat = (bpp_ == 24) ? GL_RGB : GL_RGBA;
    uint32_t size = (bpp_ == 24) ? 3 : 4;
    uint32_t length = width_ * height_ * size;
    texture_->OnInitialized(buf_,width_,height_,glFormat, length, animated_);
  }
  return true;
}

bool ImageProvider::decode(uint8_t* buf,size_t buf_size,bool re_enter) {
  bool retv = false;
  bool compressed = false;
  uint8_t* in_buf = buf;
  size_t in_buf_size = buf_size;

  // =========================
  // ======== Memory =========
  // =========================
  httpCleanUp();
  ImageFactory::Dispose();

  bool astc = isASTC(in_buf, in_buf_size);
  bool pkm = isPKM(in_buf, in_buf_size);
  bool ktx = isKTX(in_buf, in_buf_size);
  bool pvr = isPVR(in_buf, in_buf_size);
  // Compressed Texture
  if (astc || pkm || ktx || pvr) {
    compressed = true;
    if (astc) { retv = decodeASTC(in_buf, in_buf_size); }
    else if (pkm) { retv = decodePKM(in_buf, in_buf_size); }
    else if (ktx) { retv = decodeKTX(in_buf, in_buf_size); }
    else if (pvr) { retv = decodePVR(in_buf, in_buf_size); }
  }
  // Animated GIF
  else if (isGIF(in_buf,in_buf_size)) {
    retv = decodeAnimationBuf(in_buf, in_buf_size, "image/gif");
    animated_ = (getAnimationDelay() > 0) ? true : false;
  }
  // Animated PNG
  else if (isPNG(in_buf, in_buf_size) &&
           isAPNG(in_buf, in_buf_size)) {
    retv = decodeAnimationBuf(in_buf, in_buf_size, "image/apng");
    animated_ = true;
  }
  // Animated WebP
  else if (isWebP(in_buf, in_buf_size) &&
           isAnimatedWebP(in_buf, in_buf_size)) {
    retv = decodeAnimationBuf(in_buf, in_buf_size, "image/webp");
    animated_ = true;
  }
  // JPG,PNG,WEBP,SVG
  else {
    retv = decodeBuf(in_buf, in_buf_size, 0, cache_width_, cache_height_);
    if (retv && texture_->IsRun()) {
      fitScaleDown(cache_width_, cache_height_);
      if (bpp_ == 32) {
        premultiply();
      }
    }
  }

  // =========================
  // ======= FINALIZE ========
  // =========================
  if (!retv) {
    httpCleanUp();
    ImageFactory::Dispose();
    texture_->OnError(-2, " memory decode fail");
    return false;
  }
  if (!texture_->IsRun()) {
    httpCleanUp();
    ImageFactory::Dispose();
    return false;
  }

  if (!compressed) {
    uint32_t glFormat = (bpp_ == 24) ? GL_RGB : GL_RGBA;
    uint32_t size = (bpp_ == 24) ? 3 : 4;
    uint32_t length = width_ * height_ * size;
    texture_->OnInitialized(buf_,width_,height_,glFormat, length, animated_);
  }
  return true;
}

////////////////////////////////////////////////////////////////
// ASTC
// ASTC_User_Guide_102162_0001_01.pdf page 32.
// https://registry.khronos.org/OpenGL/extensions/KHR/KHR_texture_compression_astc_hdr.txt
////////////////////////////////////////////////////////////////

// COMPRESSED_RGBA
#define ASTC_4x4   0x93B0
#define ASTC_5x4   0x93B1
#define ASTC_5x5   0x93B2
#define ASTC_6x5   0x93B3
#define ASTC_6x6   0x93B4
#define ASTC_8x5   0x93B5
#define ASTC_8x6   0x93B6
#define ASTC_8x8   0x93B7
#define ASTC_10x5  0x93B8
#define ASTC_10x6  0x93B9
#define ASTC_10x8  0x93BA
#define ASTC_10x10 0x93BB
#define ASTC_12x10 0x93BC
#define ASTC_12x12 0x93BD

// COMPRESSED_SRGB8_ALPHA8
#define S_ASTC_4x4   0x93D0
#define S_ASTC_5x4   0x93D1
#define S_ASTC_5x5   0x93D2
#define S_ASTC_6x5   0x93D3
#define S_ASTC_6x6   0x93D4
#define S_ASTC_8x5   0x93D5
#define S_ASTC_8x6   0x93D6
#define S_ASTC_8x8   0x93D7
#define S_ASTC_10x5  0x93D8
#define S_ASTC_10x6  0x93D9
#define S_ASTC_10x8  0x93DA
#define S_ASTC_10x10 0x93DB
#define S_ASTC_12x10 0x93DC
#define S_ASTC_12x12 0x93DD

bool ImageProvider::decodeASTC(uint8_t* buf,size_t len) {
  struct astc_header {
    uint8_t magic[4];
    uint8_t bx;
    uint8_t by;
    uint8_t bz;
    uint8_t x[3];
    uint8_t y[3];
    uint8_t z[3];
  };
  struct astc_header* h = (struct astc_header*)buf;

  // BLOCK SIZE
  uint32_t bx = h->bx;
  uint32_t by = h->by;
  uint32_t bz = h->bz;
  if (bz != 1) {
    LogError(TAG) << "ASTC block size(" << bx << "x" << by << "x" << bz << ") error";
    return false;
  }

  // Format
  uint32_t glFormat = 0;
  glFormat = (bx == 4 && by == 4)   ? ASTC_4x4   : glFormat;
  glFormat = (bx == 5 && by == 4)   ? ASTC_5x4   : glFormat;
  glFormat = (bx == 5 && by == 5)   ? ASTC_5x5   : glFormat;
  glFormat = (bx == 6 && by == 5)   ? ASTC_6x5   : glFormat;
  glFormat = (bx == 6 && by == 6)   ? ASTC_6x6   : glFormat;
  glFormat = (bx == 8 && by == 5)   ? ASTC_8x5   : glFormat;
  glFormat = (bx == 8 && by == 6)   ? ASTC_8x6   : glFormat;
  glFormat = (bx == 8 && by == 8)   ? ASTC_8x8   : glFormat;
  glFormat = (bx == 10 && by == 5)  ? ASTC_10x5  : glFormat;
  glFormat = (bx == 10 && by == 6)  ? ASTC_10x6  : glFormat;
  glFormat = (bx == 10 && by == 8)  ? ASTC_10x8  : glFormat;
  glFormat = (bx == 10 && by == 10) ? ASTC_10x10 : glFormat;
  glFormat = (bx == 12 && by == 10) ? ASTC_12x10 : glFormat;
  glFormat = (bx == 12 && by == 12) ? ASTC_12x12 : glFormat;
  if (glFormat == 0) {
    LogError(TAG) << "ASTC block size(" << bx << "x" << by << "x" << bz << ") error";
    return false;
  }

  // WIDTH/HEIGHT
  uint32_t x = h->x[0] + (h->x[1] << 8) + (h->x[2] << 16);
  uint32_t y = h->y[0] + (h->y[1] << 8) + (h->y[2] << 16);
  uint32_t z = h->z[0] + (h->z[1] << 8) + (h->z[2] << 16);

  // LENGTH
  uint64_t required64 = (uint64_t)((x + bx-1)/bx) * ((y + by-1)/by) * ((z + bz-1)/bz) << 4;

  size_t header_size = sizeof(struct astc_header);
  if ((required64 + header_size) > len) {
    LogError(TAG) << "ASTC required length is "
                  << required64 + header_size
                  << ",buf only " << len << " !!";
    return false;
  }
  uint32_t required = (uint32_t)required64;

  uint32_t width = x;
  uint32_t height = y;
  if (saved_width_ > 0 && saved_height_ > 0) {
    if (width != saved_width_ || height != saved_height_) {
      width = saved_width_;
      height = saved_height_;
    }
  }
  texture_->OnInitialized(buf + header_size,width,height,glFormat,required,false);
  return true;
}

////////////////////////////////////////////////////////////////
// ETC1 & ETC2
// https://registry.khronos.org/webgl/extensions/WEBGL_compressed_texture_etc1/
// https://registry.khronos.org/webgl/extensions/WEBGL_compressed_texture_etc/
////////////////////////////////////////////////////////////////
// ETC1
#define ETC1_RGB8_OES                  0x8D64

// ETC2
#define R11_EAC                        0x9270
#define SIGNED_R11_EAC                 0x9271
#define RG11_EAC                       0x9272
#define SIGNED_RG11_EAC                0x9273
#define RGB8_ETC2                      0x9274
#define SRGB8_ETC2                     0x9275
#define RGB8_PUNCHTHROUGH_ALPHA1_ETC2  0x9276
#define SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define RGBA8_ETC2_EAC                 0x9278
#define SRGB8_ALPHA8_ETC2_EAC          0x9279

static uint32_t getInt16(uint8_t* x,bool big = true) {
  return big ? (x[0] << 8) | x[1] :
               (x[1] << 8) | x[0];
}

static uint32_t getInt32(uint8_t* x, bool big) {
  return big ? (x[0] << 24) | (x[1] << 16) | (x[2] << 8) | x[3] :
               (x[3] << 24) | (x[2] << 16) | (x[1] << 8) | x[0] ;
}

////////////////////////////////////////////////////////////////
// PKM file format for ETC1 & ETC2
// https://www.javatips.net/api/Rajawali-master/rajawali/src/main/java/org/rajawali3d/materials/textures/utils/ETC2Util.java
////////////////////////////////////////////////////////////////
bool ImageProvider::decodePKM(uint8_t* buf,size_t len) {
  const uint32_t pkm_formats[] = {
    ETC1_RGB8_OES,
    RGB8_ETC2,
    0,
    RGBA8_ETC2_EAC,

    RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    R11_EAC,
    RG11_EAC,
    SIGNED_R11_EAC,

    SIGNED_RG11_EAC,
  };

  struct pkm_header {
    uint8_t magic[6];
    uint8_t format[2];
    uint8_t ew[2];
    uint8_t eh[2];
    uint8_t w[2];
    uint8_t h[2];
  };

  struct pkm_header* h = (struct pkm_header*)buf;

  // Format
  size_t format = getInt16(h->format);
  if (format >= sizeof(pkm_formats)/sizeof(uint32_t)) {
    LogError(TAG) << "PKM header failed format check. format:" << format;
    return false;
  }
  uint32_t glFormat = pkm_formats[format];
  if (glFormat == 0) {
    LogError(TAG) << "PKM header format[" << format << "] error";
    return false;
  }

  // WH
  uint32_t ew = getInt16(h->ew);
  uint32_t eh = getInt16(h->eh);
  uint32_t width = getInt16(h->w);
  uint32_t height = getInt16(h->h);
  if (ew < width || (ew - width) > 4) {
    LogError(TAG) << "PKM header failed width check. Encoded:"
                  << ew << ", Actual:" << width;
    return false;
  }
  if (eh < height || (eh - height) > 4) {
    LogError(TAG) << "PKM header failed height check. Encoded:"
                  << eh << ", Actual:" << height;
    return false;
  }

  size_t header_size = sizeof(struct pkm_header);

  uint32_t required = len - header_size;
  if (glFormat == ETC1_RGB8_OES || glFormat == RGB8_ETC2) {
    required = (width * height)>>1;
  } else if (glFormat == RGBA8_ETC2_EAC) {
    required = width * height;
  }

  if (saved_width_ > 0 && saved_height_ > 0) {
    if (width != saved_width_ || height != saved_height_) {
      width = saved_width_;
      height = saved_height_;
    }
  }
  texture_->OnInitialized(buf + header_size,width,height,glFormat,required,false);
  return true;
}

////////////////////////////////////////////////////////////////
// KTX file format for ETC1 & ETC2
// https://registry.khronos.org/KTX/specs/1.0/ktxspec.v1.html
////////////////////////////////////////////////////////////////
static bool isBigEndian(uint8_t* endianess) {
  return (endianess[0] == 0x04 &&
          endianess[1] == 0x03 &&
          endianess[2] == 0x02 &&
          endianess[3] == 0x01) ? true : false;
}

bool ImageProvider::decodeKTX(uint8_t* buf,size_t len) {
  struct ktx_header {
    uint8_t magic[12];
    uint8_t endianess[4];

    uint8_t glType[4];
    uint8_t glTypeSize[4];
    uint8_t glFormat[4];
    uint8_t glInternalFormat[4];

    uint8_t glBaseInternalFormat[4];
    uint8_t pixelWidth[4];
    uint8_t pixelHeight[4];
    uint8_t pixelDepth[4];

    uint8_t numOfArrayElement[4];
    uint8_t numOfFaces[4];
    uint8_t numOfMipmalLevels[4];
    uint8_t byteOfKeyValueData[4];
  };

  struct ktx_header* h = (struct ktx_header*)buf;
  bool big = isBigEndian(h->endianess);

  // Format
  uint32_t glFormat = getInt32(h->glInternalFormat,big);
  if (glFormat == 0) {
    LogError(TAG) << "KTX glFormat error";
    return false;
  }

  // WH
  uint32_t width = getInt32(h->pixelWidth,big);
  uint32_t height = getInt32(h->pixelHeight,big);

  // Meta Size
  size_t header_size = sizeof(struct ktx_header);
  header_size += getInt32(h->byteOfKeyValueData,big);

  // LENGTH
  uint8_t* required_ptr = buf + header_size;
  uint32_t required = getInt32(required_ptr,big);
  header_size += sizeof(uint32_t);

  if ((required + header_size) > len) {
    LogError(TAG) << "KTX required length is "
                  << required + header_size
                  << ",buf only " << len << " !!";
    return false;
  }

  if (saved_width_ > 0 && saved_height_ > 0) {
    if (width != saved_width_ || height != saved_height_) {
      width = saved_width_;
      height = saved_height_;
    }
  }
  texture_->OnInitialized(buf + header_size,width,height,glFormat,required,false);
  return true;
}

////////////////////////////////////////////////////////////////
// PVR file format for ETC1 & ETC2
// http://powervr-graphics.github.io/WebGL_SDK/WebGL_SDK/Documentation/Specifications/PVR%20File%20Format.Specification.pdf
////////////////////////////////////////////////////////////////
bool ImageProvider::decodePVR(uint8_t* buf,size_t len) {
  struct pvr_header {
    uint32_t version;
    uint32_t flags;
    uint32_t pixelFormat;
    uint32_t dummy;
    uint32_t colorSpace;
    uint32_t channelType;
    uint32_t height;
    uint32_t width;
    uint32_t depth;
    uint32_t numSurfaces;
    uint32_t numFaces;
    uint32_t mipmapCount;
    uint32_t metaSize;
  };

  struct pvr_header* h = (struct pvr_header*)buf;

  // Format
  uint32_t glFormat = 0;
  glFormat = (h->pixelFormat == 6) ? ETC1_RGB8_OES :  glFormat;
  glFormat = (h->pixelFormat == 22) ? RGB8_ETC2 : glFormat;
  glFormat = (h->pixelFormat == 23) ? RGBA8_ETC2_EAC : glFormat;
  glFormat = (h->pixelFormat == 24) ? RGB8_PUNCHTHROUGH_ALPHA1_ETC2 : glFormat;
  glFormat = (h->pixelFormat == 27) ? ASTC_4x4 : glFormat;
  glFormat = (h->pixelFormat == 28) ? ASTC_5x4 : glFormat;
  glFormat = (h->pixelFormat == 29) ? ASTC_5x5 : glFormat;
  glFormat = (h->pixelFormat == 30) ? ASTC_6x5 : glFormat;
  glFormat = (h->pixelFormat == 31) ? ASTC_6x6 : glFormat;
  glFormat = (h->pixelFormat == 32) ? ASTC_8x5 : glFormat;
  glFormat = (h->pixelFormat == 33) ? ASTC_8x6 : glFormat;
  glFormat = (h->pixelFormat == 34) ? ASTC_8x8 : glFormat;
  glFormat = (h->pixelFormat == 35) ? ASTC_10x5 : glFormat;
  glFormat = (h->pixelFormat == 36) ? ASTC_10x6 : glFormat;
  glFormat = (h->pixelFormat == 37) ? ASTC_10x8 : glFormat;
  glFormat = (h->pixelFormat == 38) ? ASTC_10x10 : glFormat;
  glFormat = (h->pixelFormat == 39) ? ASTC_12x10 : glFormat;
  glFormat = (h->pixelFormat == 40) ? ASTC_12x12 : glFormat;
  if (glFormat == 0) {
    LogError(TAG) << "PVR glFormat error";
    return false;
  }

  // WH
  uint32_t width = h->width;
  uint32_t height = h->height;

  // Meta Size
  size_t header_size = sizeof(struct pvr_header);
  header_size += h->metaSize;

  uint32_t required = len - header_size;
  if (glFormat == ETC1_RGB8_OES || glFormat == RGB8_ETC2) {
    required = (width * height)>>1;
  } else if (glFormat == RGBA8_ETC2_EAC) {
    required = width * height;
  }

  if (saved_width_ > 0 && saved_height_ > 0) {
    if (width != saved_width_ || height != saved_height_) {
      width = saved_width_;
      height = saved_height_;
    }
  }
  texture_->OnInitialized(buf+header_size,width,height,glFormat,required,false);
  return true;
}

////////////////////////////////////////////////////////////////
// Animated GIF
////////////////////////////////////////////////////////////////
int ImageProvider::onAnimationFrame(void* data) {
  ImageProvider* self = (ImageProvider*)data;
  {
    std::lock_guard<std::recursive_mutex> autolock(self->mutex_);
    self->timer_source_ = nullptr;
  }
  self->execAnimationFrame(true);
  return G_SOURCE_REMOVE;
}

////////////////////////////////////////////////////////////////
// FILE
////////////////////////////////////////////////////////////////
std::tuple<int,std::string> ImageProvider::file(const std::string& url) {
  httpCleanUp();

  int errCode = 0;
  std::string errText;

  std::ifstream is(url, std::ios::binary);
  if (is.is_open()) {
    size_t size;
    std::string mtime;
    std::tie(size,mtime) = ImageCache::get_fileinfo(url);

    http_buf_size_ = size;
    http_buf_ = (uint8_t*)::malloc(http_buf_size_);
    is.read((char*)http_buf_,http_buf_size_);
    is.close();

    http_content_length_ = size;
    http_etag_ = mtime;
    http_can_cache_ = true;
    http_no_transform_ = false;
  } else {
    httpCleanUp();
    if (texture_->IsRun()) {
      errCode = 404;
      errText = "file ";
      errText += url;
      errText += " not found";
    }
  }
  return std::make_tuple(errCode,errText);
}

////////////////////////////////////////////////////////////////
// HTTP
////////////////////////////////////////////////////////////////
size_t ImageProvider::onHttpBody(void *ptr, size_t size, size_t count, void *data) {
  ImageProvider* self = (ImageProvider*)data;
  if (!self->texture_->IsRun()) {
    self->httpCleanUp();
    return 0;
  }
  size_t sz = size * count;

  PacketInfo info { (uint8_t*)::malloc(sz), sz };
  ::memcpy(info.data,ptr,sz);
  self->http_packets_.push_back(info);
  return sz;
}

static size_t onHttpHead(char* ptr, size_t size, size_t count, void* data) {
  std::string* response = (std::string*)data;
  *response += ptr;
  return size * count;
}

std::tuple<int,std::string> ImageProvider::http(const std::string& url) {
  httpCleanUp();

  int errCode = 0;
  std::string errText;
  int64_t start_tick = getCurTick();

  // NETWOR
  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    LogWarn(TAG) << "CURL: init error";
    errCode = CURLE_FAILED_INIT;
    errText = "Curl init error";
    return std::make_tuple(errCode,errText);
  }

  // Initialize
  std::string header_response;
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "flutter ImageTexture");
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onHttpBody);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, NETWORK_CONNECTION_TIMEOUT_SEC);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, NETWORK_TIMEOUT_MS);

  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, onHttpHead);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_response);

  CURLcode res = CURLE_OK;
  while (texture_->IsRun()) {
    res = curl_easy_perform(curl);
    if (getDiffTick(start_tick) > NETWORK_TIMEOUT_MS) {
      break;
    }
    if (res != CURLE_OPERATION_TIMEDOUT &&
        res != CURLE_COULDNT_CONNECT) {
      break;
    }

    std::string err = curl_easy_strerror(res);
    LogWarn(TAG) << "CURL: retry "
                 << err << " "
                 << url;
    httpCleanUp();
    header_response.clear();
  }

  // Finalize
  if (res == CURLE_OK) {
    int size = 0;
    for (auto& info : http_packets_) {
      size += info.size;
    }
    http_buf_ = (uint8_t*)::malloc(size);
    http_buf_size_ = size;
    http_content_length_ = ImageCache::get_contentLength(header_response);
    http_etag_ = ImageCache::get_etag(header_response);
    std::string cache_control = ImageCache::get_cacheControl(header_response);

    // reset http_buf_size_, if less than http_content_length_
    if (http_content_length_ > 0 &&
        http_buf_size_ > http_content_length_) {
      http_buf_size_ = http_content_length_;
    }

    uint8_t* buf = http_buf_;
    for (auto& info : http_packets_) {
      ::memcpy(buf,info.data,info.size);
      ::free(info.data);
      buf += info.size;
    }
    http_packets_.clear();

    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code > 300) {
      errCode = response_code;
      errText = "server error";
    }

    // Debug..
    char *ct = NULL;
    double start = 0;
    double speed = 0;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
    curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &start);
    curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &speed);
    std::string content_type = ct ? ct : "none";
    LogDebug(TAG) << "CURL: res(" << response_code
                 << "," << content_type << ")"
                 << (int)(start*1000) << ":"
                 << getDiffTick(start_tick) << " msec,"
                 << (int)speed/1000 << " KB/S,"
                 << http_buf_size_/1024 << " KB, "
                 << url;

    http_can_cache_ = true;
    if (!cache_control.empty()) {
      bool debug = false;
      if (cache_control.find("no-transform") != std::string::npos) {
        http_no_transform_ = true;
        debug = true;
      }
      if (cache_control.find("no-cache") != std::string::npos ||
         (cache_control.find("no-store") != std::string::npos)) {
        http_can_cache_ = false;
        debug = true;
      }
      if (debug) {
        LogInfo(TAG) << "CURL: cache-control: "
                     << cache_control << " "
                     << url;
      }
    }
  } else {
    httpCleanUp();
    if (texture_->IsRun()) {
      errCode = (int)res;
      errText = curl_easy_strerror(res);
      LogWarn(TAG) << "CURL: error code " << errCode
                   << "(" << errText << ").."
                   << url;
    }
  }

  curl_easy_cleanup(curl);
  return std::make_tuple(errCode,errText);
}

void ImageProvider::httpCleanUp() {
  if (http_buf_) {
    ::free(http_buf_);
    http_buf_ = nullptr;
    http_buf_size_ = 0;
  }
  for (auto& info : http_packets_) {
    ::free(info.data);
  }
  http_packets_.clear();

  http_content_length_ = 0;
  http_etag_.clear();
  http_can_cache_ = false;
  http_no_transform_ = false;
  saved_width_ = 0;
  saved_height_ = 0;
}

void ImageProvider::pushEvent(EventType event) {
  std::lock_guard<std::recursive_mutex> autolock(mutex_);
  events_.push_back(event);

  if (idle_source_ == nullptr) {
    idle_source_ = attachToGMainLoop([](void* data) -> int {
      ImageProvider* self = (ImageProvider*)data;
      self->popEvents();
      return G_SOURCE_REMOVE;
    }, this);
  }
}

void ImageProvider::popEvents() {
  std::vector<EventType> events;
  {
    std::lock_guard<std::recursive_mutex> autoLock(mutex_);
    idle_source_ = nullptr;
    std::swap(events_, events);
  }

  for (auto& type : events) {
    switch(type) {
      case kTypeDecodeUrl: execDecodeUrl(); break;
      case kTypeDecodeBuf: execDecodeBuf(); break;
      case kTypeUploaded:  execUploaded(); break;
      case kTypeQuit:      execQuit(); break;
      case kTypePlay:      execPlay(); break;
      case kTypeStop:      execStop(); break;
      default: break;
    }
  }
}

////////////////////////////////////////////////////////////////
// GMainLoop
////////////////////////////////////////////////////////////////
GSource* ImageProvider::attachToGMainLoop(GMainLoopCallback cb,void* data,int64_t delayed) {
  std::lock_guard<std::recursive_mutex> autolock(mutex_);
  GSource* src = nullptr;
  if (g_loop_) {
    src = (delayed > 0) ? g_timeout_source_new(delayed) : g_idle_source_new();
    g_source_set_callback(src,cb,data,NULL);
    g_source_attach(src, g_main_loop_get_context(g_loop_));
    g_source_unref(src);
  }
  return src;
}

////////////////////////////////////////////////////////////////
// Time utils
////////////////////////////////////////////////////////////////
static int64_t getCurTick() {
  auto var0 = std::chrono::steady_clock::now();
  auto var1 = std::chrono::time_point_cast<std::chrono::milliseconds>(var0).time_since_epoch();
  int64_t msec = std::chrono::duration_cast<std::chrono::milliseconds>(var1).count();
  return msec;
}

static int64_t getDiffTick(int64_t t0) {
  int64_t t1 = getCurTick();
  return std::abs(t1-t0);
}
