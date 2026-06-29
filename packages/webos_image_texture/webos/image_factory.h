// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: LicenseRef-LGE-Proprietary

#ifndef IMAGEFACTORY_H_
#define IMAGEFACTORY_H_
#include <string>
#include <list>
#include <functional>
#include <stdint.h>

class ImageFactory {
public:
  ImageFactory();
  virtual ~ImageFactory();

  static bool writeBMP(const std::string& path,uint8_t* buf,int width,int height,int bpp);
  static bool writePNG(const std::string& path,uint8_t* buf,int width,int height,int bpp);

protected:
  void Dispose();

  // APNG
  bool isAPNG(uint8_t* buf, size_t buf_size);

  // AnimatedWebP
  bool isAnimatedWebP(uint8_t* buf, size_t buf_size);

  // PNG,JPG,WEBP,SVG
  bool infoFile(const std::string& fileName);
  bool infoBuf(uint8_t* buf,size_t len);
  bool decodeFile(const std::string& fileName,int bpp = 0,int width = 0,int height = 0);
  bool decodeBuf(uint8_t* buf,size_t len,int bpp = 0,int width = 0,int height = 0);

  // GIF,APNG
  bool decodeAnimationFile(const std::string& fileName, const std::string& mime_type);
  bool decodeAnimationBuf(uint8_t* buf,size_t len, const std::string& mime_type);
  bool decodeAnimationNext();
  int64_t getAnimationDelay();
  int getAnimationLoopCount();
  void animationCleanUp();

  // Resize
  bool fitScaleDown(const int width,const int height);
  bool fitCover(const int width,const int height);

  // Premultiply
  void premultiply();

  uint8_t* buf_ = nullptr;
  int width_ = 0;
  int height_ = 0;
  int bpp_ = 0;

private:
  void setAnimationFn(const std::string& mime_type);

  bool decodeAnimationGIF_File(const std::string& fileName);
  bool decodeAnimationGIF_Buf(uint8_t* buf,size_t len);
  bool decodeAnimationGIF_Next();
  int64_t getAnimationGIF_Delay();
  void animationGIF_CleanUp();

  bool decodeAnimationPNG_File(const std::string& fileName);
  bool decodeAnimationPNG_Buf(uint8_t* buf,size_t len);
  bool decodeAnimationPNG_Next();
  int64_t getAnimationPNG_Delay();
  void animationPNG_CleanUp();
  int64_t frame_duration_ = 0;

  bool decodeAnimationWebP_File(const std::string& fileName);
  bool decodeAnimationWebP_Buf(uint8_t* buf,size_t len);
  bool decodeAnimationWebP_Next();
  int64_t getAnimationWebP_Delay();
  void animationWebP_CleanUp();
  int prev_frame_timestamp_ = 0;

  void* animation_context_ = nullptr;
  std::string animation_file_name_;
  uint8_t* animation_buf_ = nullptr;
  size_t animation_buf_size_ = 0;
  int animation_loop_count_ = 1;

  std::function<bool(const std::string&)> animation_file_fn_ = nullptr;
  std::function<bool(uint8_t* buf, size_t len)> animation_buf_fn_ = nullptr;
  std::function<bool()> animation_next_fn_ = nullptr;
  std::function<int64_t()> animation_frame_duration_fn_ = nullptr;
  std::function<void()> animation_clean_up_fn_ = nullptr;

  bool webp_ = false;
  bool svg_ = false;
};
#endif
