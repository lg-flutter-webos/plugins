// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: LicenseRef-LGE-Proprietary

#ifndef IMAGEPROVIDER_H_
#define IMAGEPROVIDER_H_
#include <string>
#include <list>
#include <tuple>
#include <mutex>

#include <stdint.h>
#include <glib.h>
#include "image_factory.h"

class ImageTexture;

class ImageProvider : public ImageFactory {
public:
  ImageProvider(ImageTexture* texture);
  virtual ~ImageProvider();

  void Event();
  void Decode(const std::string& url,
              int cache_width,int cache_height);
  void Decode(uint8_t* buf,size_t length,
              int cache_width,int cache_height);
  void OnUploaded();

private:
   enum EventType {
    kTypeDecodeUrl,
    kTypeDecodeBuf,
    kTypeUploaded,
    kTypeQuit,
    kTypePlay,
    kTypeStop
  };

  void execPlay();
  void execStop();
  void execAnimationFrame(bool frame);
  void execQuit();
  void execDecodeUrl();
  void execDecodeBuf();
  void execUploaded();

  void pushEvent(EventType event);
  void popEvents();

  bool decode(const std::string& url, bool re_enter = false);
  bool decode(uint8_t* buf,size_t legnth, bool re_enter = false);

  //==== File ====
  std::tuple<int,std::string> file(const std::string& uri);

  //==== Http ====
  const int64_t NETWORK_CONNECTION_TIMEOUT_SEC = 2;
  const int64_t NETWORK_TIMEOUT_MS = 30*1000;

  typedef struct {
    uint8_t* data;
    size_t size;
  } PacketInfo;

  static size_t onHttpBody(void* ptr, size_t size, size_t count, void* data);
  std::tuple<int,std::string> http(const std::string& uri);
  void httpCleanUp();

  uint8_t* http_buf_ = nullptr;
  size_t http_buf_size_ = 0;
  size_t http_content_length_ = 0;
  std::string http_etag_;
  std::list<PacketInfo> http_packets_;
  bool http_can_cache_ = false;
  bool http_no_transform_ = false;

  //=== Cache ===
  int saved_width_ = 0;
  int saved_height_ = 0;

  //==== Buffer ====
  uint8_t* in_buf_ = nullptr;
  size_t in_buf_size_ = 0;

  //==== GIF ====
  static int onAnimationFrame(void* data);
  static int onAnimationStop(void* data);
  bool animated_ = false;
  int animation_cnt_ = 0;

  //==== Resize ====
  int cache_width_ = 0;
  int cache_height_ = 0;

  //==== Compressed Texture (ASTC,ETC1,ETC2) ====
  bool decodeASTC(uint8_t* buf,size_t len);
  bool decodePKM(uint8_t* buf,size_t len);
  bool decodeKTX(uint8_t* buf,size_t len);
  bool decodePVR(uint8_t* buf,size_t len);

  //==== ImageTexture ====
  std::string url_;
  ImageTexture* texture_ = nullptr;

  //==== GMainLoop ====
  typedef int (*GMainLoopCallback)(void*);
  GSource* attachToGMainLoop(GMainLoopCallback cb,void* data,int64_t delayed = 0);

  GMainLoop* g_loop_ = nullptr;
  GSource* idle_source_ = nullptr;
  GSource* timer_source_ = nullptr;
  std::recursive_mutex mutex_;
  std::vector<EventType> events_;
};
#endif
