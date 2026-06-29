// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: LicenseRef-LGE-Proprietary

#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include "log.h"
#include "image_factory.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "ImageFactory::"

#define MAX_PIXELS (1 << 23) // 8M pixels
#define MAX_WH 1920
#define SCALEDOWN_MIN_DIMENSION (480*270)

///// STB config
#define STBI_ASSERT(x) { if ((bool)(x) == false) throw -1; }
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_GIF
#define STBI_ONLY_BMP
#ifdef __ARM_NEON
#define STBI_NEON
#endif

#include "third_party/stb/stb_image.h"
#include "third_party/uc_apng_loader/uc_apng_loader.h"

////// LunaSVG
#include "third_party/lunasvg/include/lunasvg.h"

static bool svg_info(const char* fname,int* width,int* height,int* comp) {
  auto document = lunasvg::Document::loadFromFile(fname);
  if (!document) {
    return false;
  }

  *width = std::round(document->width());
  *height = std::round(document->height());
  *comp = 4;
  return true;
}

static bool svg_info_from_memory(uint8_t* buf,size_t len,int* width,int* height,int* comp) {
  auto document = lunasvg::Document::loadFromData((const char*)buf,len);
  if (!document) {
    return false;
  }
  *width = std::round(document->width());
  *height = std::round(document->height());
  *comp = 4;
  return true;
}

static uint8_t* svg_load_common(std::unique_ptr<lunasvg::Document> document,
                                int* width,int* height,int* comp,
                                int required_width,int required_height) {
  double w = document->width();
  double h = document->height();
  double scale = 1.0f;
  if (required_width > 0 && required_height > 0) {
    double x_scale = (float)required_width/w;
    double y_scale = (float)required_height/h;
    scale = std::min(x_scale,y_scale);
  } else if (required_width > 0 || required_height > 0) {
    scale = (required_width > 0) ? required_width/w : required_height/h;
  }

  int wi = std::round(w*scale);
  int hi = std::round(h*scale);
  uint8_t* img = (uint8_t*)::malloc(wi*hi*4);
  auto bitmap = lunasvg::Bitmap(img,wi,hi,wi*4);
  bitmap.clear(0);

  lunasvg::Matrix matrix = lunasvg::Matrix{};
  matrix.scale(scale,scale);
  document->render(bitmap,matrix);
  bitmap.convertToRGBA();

  *width = wi;
  *height = hi;
  *comp = 4;
  return img;
}

static uint8_t* svg_load(const char* fname,int* width,int* height,int* comp,
                         int required_width,int required_height) {
  auto document = lunasvg::Document::loadFromFile(fname);
  if (!document) {
    LogWarn(TAG) << "svg_load fail";
    return nullptr;
  }
  return svg_load_common(std::move(document),width,height,comp,required_width,required_height);
}

static uint8_t* svg_load_from_memory(uint8_t* buf,size_t len,int* width,int* height,int* comp,
                                     int required_width,int required_height) {
  auto document = lunasvg::Document::loadFromData((const char*)buf,len);
  if (!document) {
    LogWarn(TAG) << "svg_load_from_memory fail";
    return nullptr;
  }
  return svg_load_common(std::move(document),width,height,comp,required_width,required_height);
}

////// WebP
#include "webp/decode.h"

static bool webp_info_from_memory(uint8_t* buf,size_t len,int* width,int* height,int* comp) {
  struct WebPBitstreamFeatures features;
  if (WebPGetFeatures(buf,len,&features) == VP8_STATUS_OK) {
    *width = features.width;
    *height = features.height;
    *comp = features.has_alpha ? 4 : 3;
    return true;
  }
  return false;
}

static bool webp_info(const char* fname,int* width,int* height,int* comp) {
  FILE* fp = ::fopen(fname,"r");
  if (fp == nullptr) {
    return false;
  }

  ::fseek(fp,0,SEEK_END);
  size_t len = ::ftell(fp);
  ::fseek(fp,0,SEEK_SET);
  uint8_t* buf = (uint8_t*)::malloc(len);
  ::fread(buf,1,len,fp);

  bool ok = webp_info_from_memory(buf,len,width,height,comp);
  ::free(buf);
  ::fclose(fp);
  return ok;
}

static uint8_t* webp_load_from_memory(uint8_t* buf,size_t len,int* width,int* height,int* comp,int req_comp) {
  if (req_comp == 0) {
    struct WebPBitstreamFeatures features;
    if (WebPGetFeatures(buf,len,&features) == VP8_STATUS_OK) {
      req_comp = features.has_alpha ? 4 : 3;
    }
  }
  *comp = req_comp;
  uint8_t* img = (req_comp == 4) ? WebPDecodeRGBA(buf,len,width,height) :
                                   WebPDecodeRGB(buf,len,width,height);
  return img;
}

static uint8_t* webp_load(const char* fname,int* width,int* height,int* comp,int req_comp) {
  FILE* fp = ::fopen(fname,"r");
  if (fp == nullptr) {
    return nullptr;
  }

  ::fseek(fp,0,SEEK_END);
  size_t len = ::ftell(fp);
  ::fseek(fp,0,SEEK_SET);
  uint8_t* buf = (uint8_t*)::malloc(len);
  ::fread(buf,1,len,fp);
  ::fclose(fp);

  uint8_t* img = webp_load_from_memory(buf,len,width,height,comp,req_comp);
  ::free(buf);
  return img;
}

////////////////////////////////////////////////////////////////
// Common
////////////////////////////////////////////////////////////////
static int64_t getCurTick();
static int64_t getDiffTick(int64_t t0);

ImageFactory::ImageFactory() {
  static bool s_first = true;
  if (s_first) {
    s_first = false;
#ifdef STBI_NEON
    LogInfo(TAG) << "__ARM_NEON defined";
#endif
  }
}

ImageFactory::~ImageFactory() {
  Dispose();
}

////////////////////////////////////////////////////////////////
// Dispose
////////////////////////////////////////////////////////////////
void ImageFactory::Dispose() {
  animationCleanUp();

  if (buf_) {
    if (webp_) {
      WebPFree(buf_);
    } else if (svg_) {
      ::free(buf_);
    } else {
      STBI_FREE(buf_);
    }
    buf_ = nullptr;
  }
}

////////////////////////////////////////////////////////////////
// JPG,PNG,WebP
////////////////////////////////////////////////////////////////
//     BPP         components
//       8           grey
//       16          grey,alpha
//       24          red, green, blue
//       32          red, green, blue, alpha
static int getSuitableBpp(int bpp) {
  if (bpp == 8) return 24;
  if (bpp == 16) return 32;
  return bpp;
}

bool ImageFactory::infoFile(const std::string& fileName) {
  try {
    webp_ = false;
    svg_ = false;
    int comp;

    if (!stbi_info(fileName.c_str(),&width_,&height_,&comp) &&
        !(webp_ = webp_info(fileName.c_str(),&width_,&height_,&comp)) &&
        !(svg_ = svg_info(fileName.c_str(),&width_,&height_,&comp))) {
      return false;
    }
    bpp_ = comp*8;
  } catch (...) {
    LogError(TAG) << "infoFile(" << fileName << ") error";
    return false;
  }

  if (width_ * height_ > MAX_PIXELS) {
    LogError(TAG) << "Too large to decode : " << width_ << "x" << height_;
    return false;
  }
  return true;
}

bool ImageFactory::infoBuf(uint8_t* buf,size_t len) {
  try {
    webp_ = false;
    svg_ = false;
    int comp;

    if (!stbi_info_from_memory(buf,len,&width_,&height_,&comp) &&
        !(webp_ = webp_info_from_memory(buf,len,&width_,&height_,&comp)) &&
        !(svg_ = svg_info_from_memory(buf,len,&width_,&height_,&comp))) {
      return false;
    }
    bpp_ = comp*8;
  } catch (...) {
    LogError(TAG) << "infoBuf error";
    return false;
  }

  if (width_ * height_ > MAX_PIXELS) {
    LogError(TAG) << "Too large to decode : " << width_ << "x" << height_;
    return false;
  }
  return true;
}

bool ImageFactory::decodeFile(const std::string& fileName,int bpp,int width,int height) {
  try {
    if (infoFile(fileName) == false) {
      LogWarn(TAG) << "decodeFile(" << fileName << ") fail";
      return false;
    }

    // bpp_
    if (bpp == 0) {
      bpp = getSuitableBpp(bpp_);
    }
    if (bpp < 24) {
      LogWarn(TAG) << fileName << "'s bpp is " << bpp;
      return false;
    }
    bpp_ = bpp;

    // buf_
    int comp = bpp/8;
    if (webp_) {
      buf_ = webp_load(fileName.c_str(),&width_,&height_,&comp,bpp/8);
    } else if (svg_) {
      buf_ = svg_load(fileName.c_str(),&width_,&height_,&comp,width,height);
    } else {
      buf_ = stbi_load(fileName.c_str(),&width_,&height_,&comp,bpp/8);
    }
    if (!buf_) {
      LogError(TAG) << "decode(" << fileName << ") fail";
      return false;
    }
  } catch (...) {
    LogError(TAG) << "decodeFile(" << fileName << ") error";
  }
  return buf_ ? true : false;
}

bool ImageFactory::decodeBuf(uint8_t* buf,size_t len,int bpp,int width,int height) {
  try {
    if (infoBuf(buf,len) == false) {
      LogWarn(TAG) << "decodeBuf fail";
      return false;
    }

    // bpp_
    if (bpp == 0) {
      bpp = getSuitableBpp(bpp_);
    }
    if (bpp < 24) {
      LogError(TAG) << "buf size: " << len << ",  bpp is " << bpp;
      return false;
    }
    bpp_ = bpp;

    // buf_
    int comp = bpp/8;
    if (webp_) {
      buf_ = webp_load_from_memory(buf,len,&width_,&height_,&comp,bpp/8);
    } else if (svg_) {
      buf_ = svg_load_from_memory(buf,len,&width_,&height_,&comp,width,height);
    } else {
      buf_ = stbi_load_from_memory(buf,len,&width_,&height_,&comp,bpp/8);
    }
    if (!buf_) {
      LogError(TAG) << "decodeBuf[" << len << "] fail";
      return false;
    }
  } catch (...) {
    LogError(TAG) << "decodeBuf error";
  }
  return buf_ ? true : false;
}

////////////////////////////////////////////////////////////////
// Animation
////////////////////////////////////////////////////////////////
void ImageFactory::setAnimationFn(const std::string& mime_type) {
  if (mime_type == "image/gif") {
    animation_file_fn_ = std::bind(&ImageFactory::decodeAnimationGIF_File, this,
                                   std::placeholders::_1);
    animation_buf_fn_ = std::bind(&ImageFactory::decodeAnimationGIF_Buf, this,
                                  std::placeholders::_1, std::placeholders::_2);
    animation_next_fn_ = std::bind(&ImageFactory::decodeAnimationGIF_Next, this);
    animation_frame_duration_fn_ = std::bind(&ImageFactory::getAnimationGIF_Delay, this);
    animation_clean_up_fn_ = std::bind(&ImageFactory::animationGIF_CleanUp, this);
  } else if (mime_type == "image/apng") {
    animation_file_fn_ = std::bind(&ImageFactory::decodeAnimationPNG_File, this,
                                   std::placeholders::_1);
    animation_buf_fn_ = std::bind(&ImageFactory::decodeAnimationPNG_Buf, this,
                                  std::placeholders::_1, std::placeholders::_2);
    animation_next_fn_ = std::bind(&ImageFactory::decodeAnimationPNG_Next, this);
    animation_frame_duration_fn_ = std::bind(&ImageFactory::getAnimationPNG_Delay, this);
    animation_clean_up_fn_ = std::bind(&ImageFactory::animationPNG_CleanUp, this);
  } else if (mime_type == "image/webp") {
    animation_file_fn_ = std::bind(&ImageFactory::decodeAnimationWebP_File, this,
                                   std::placeholders::_1);
    animation_buf_fn_ = std::bind(&ImageFactory::decodeAnimationWebP_Buf, this,
                                  std::placeholders::_1, std::placeholders::_2);
    animation_next_fn_ = std::bind(&ImageFactory::decodeAnimationWebP_Next, this);
    animation_frame_duration_fn_ = std::bind(&ImageFactory::getAnimationWebP_Delay, this);
    animation_clean_up_fn_ = std::bind(&ImageFactory::animationWebP_CleanUp, this);
  } else {
    LogError(TAG) << "Error mime_type : " << mime_type;
  }
}

bool ImageFactory::decodeAnimationFile(const std::string& fileName,
                                       const std::string& mime_type) {
  if (!infoFile(fileName)) {
    return false;
  }

  setAnimationFn(mime_type);
  if (animation_file_fn_) {
    return animation_file_fn_(fileName);
  }
  return false;
}

bool ImageFactory::decodeAnimationBuf(uint8_t* buf, size_t len,
                                      const std::string& mime_type) {
  if (!infoBuf(buf, len)) {
    return false;
  }

  setAnimationFn(mime_type);
  if (animation_buf_fn_) {
    return animation_buf_fn_(buf,len);
  }
  return false;
}

bool ImageFactory::decodeAnimationNext() {
  if (animation_next_fn_) {
    return animation_next_fn_();
  }
  return false;
}

int64_t ImageFactory::getAnimationDelay() {
  if (animation_frame_duration_fn_) {
    return animation_frame_duration_fn_();
  }
  return 0;
}

int ImageFactory::getAnimationLoopCount() {
  return animation_loop_count_;
}

void ImageFactory::animationCleanUp() {
  if (animation_clean_up_fn_) {
    animation_clean_up_fn_();
  }
}

////////////////////////////////////////////////////////////////
// Animated GIF
////////////////////////////////////////////////////////////////
typedef struct {
  FILE *f;
  stbi__context s;
  stbi__gif g;
} AnimationContext;

bool ImageFactory::decodeAnimationGIF_File(const std::string& fileName) {
  try {
    if (animation_context_) {
      LogError(TAG) << "decodeAnimationFile context error";
    }

    AnimationContext* p = new AnimationContext;
    ::memset(p,0,sizeof(AnimationContext));
    p->g.loop_count = 1;
    animation_context_ = (void*)p;
    animation_file_name_ = fileName;
    animation_buf_ = nullptr;
    animation_buf_size_ = 0;

    if ((p->f = stbi__fopen(fileName.c_str(), "rb")) == 0) {
      LogWarn(TAG) << fileName << " is not exist";
      return false;
    }
    stbi__start_file(&p->s, p->f);
    if (!stbi__gif_test(&p->s)) {
      LogWarn(TAG) << "GIF_file " << fileName << " is not GIF";
      return false;
    }
  } catch (...) {
    LogError(TAG) << "GIF(" << fileName << ") error";
    return false;
  }
  return decodeAnimationGIF_Next();
}

bool ImageFactory::decodeAnimationGIF_Buf(uint8_t* buf,size_t len) {
  try {
    if (animation_context_) {
      LogError(TAG) << "decodeAnimationBuf context error";
    }

    AnimationContext* p = new AnimationContext;
    ::memset(p,0,sizeof(AnimationContext));
    p->g.loop_count = 1;
    animation_context_ = (void*)p;
    animation_file_name_.clear();
    animation_buf_ = buf;
    animation_buf_size_ = len;
    if (buf == nullptr || len == 0) {
      return false;
    }

    stbi__start_mem(&p->s,buf,len);
    if (!stbi__gif_test(&p->s)) {
      LogWarn(TAG) << "GIF_buf is not GIF";
      return false;
    }
  } catch (...) {
    LogError(TAG) << "GIF_buf error";
    return false;
  }
  return decodeAnimationGIF_Next();
}

bool ImageFactory::decodeAnimationGIF_Next() {
  try {
    AnimationContext *p = (AnimationContext*)animation_context_;
    if (p == nullptr) {
      bool retv = false;
      if (!animation_file_name_.empty()) {
        retv = decodeAnimationGIF_File(animation_file_name_);
      } else if (animation_buf_ != nullptr) {
        retv = decodeAnimationGIF_Buf(animation_buf_,animation_buf_size_);
      }
      return retv;
    }

    int comp = 4;     //Just for initialize, will be replaced inside STB
    int req_comp = 4; //STB doesn't be used req_comp, so this is a dummy.
    uint8_t* buf = stbi__gif_load_next(&p->s, &p->g, &comp,req_comp,nullptr,buf_);
    if (buf == (uint8_t*)&p->s) {
      animationGIF_CleanUp();
      return false;
    }
    buf_ = buf;
    bpp_ = comp*8;
    width_ = p->g.w;
    height_ = p->g.h;
    animation_loop_count_ = p->g.loop_count;
  } catch (...) {
    LogError(TAG) << "GIF_next error";
    return false;
  }
  return true;
}

int64_t ImageFactory::getAnimationGIF_Delay() {
  AnimationContext *p = (AnimationContext*)animation_context_;
  if (p == nullptr) {
    if (animation_file_name_.empty() && animation_buf_ == nullptr) {
      LogWarn(TAG) << "GIF_delay was called, but this is not a animatedGIF";
    }
    return 0;
  }
  return p->g.delay;
}

void ImageFactory::animationGIF_CleanUp() {
  AnimationContext *p = (AnimationContext*)animation_context_;
  if (p) {
    if (p->f != nullptr) ::fclose(p->f);
    if (p->g.history) STBI_FREE(p->g.history);
    if (p->g.background) STBI_FREE(p->g.background);

    delete p;
    animation_context_ = nullptr;
  }
}

////////////////////////////////////////////////////////////////
// Animated PNG
////////////////////////////////////////////////////////////////
bool ImageFactory::isAPNG(uint8_t* buf, size_t buf_size) {
  try {
    auto loader = uc::apng::create_memory_loader((const char*)buf, buf_size);
    int num_frames = loader.num_frames();
    if (num_frames > 1) {
      return true;
    }
  } catch (...) {
    LogError(TAG) << "isAPNG() error";
  }
  return false;
}

bool ImageFactory::decodeAnimationPNG_File(const std::string& fileName) {
  try {
    if (animation_context_) {
      LogError(TAG) << "decodeAnimationFile context error";
    }

    auto loader = new uc::apng::loader<std::ifstream>(
        uc::apng::make_unique<std::ifstream>(fileName.c_str(), std::ios::in | std::ios::binary));
    animation_context_ = (void*)loader;
    animation_file_name_ = fileName;
    animation_buf_ = nullptr;
    animation_buf_size_ = 0;
    animation_loop_count_ = loader->num_plays();
  } catch (...) {
    LogError(TAG) << "APNG(" << fileName << ") error";
    return false;
  }
  return decodeAnimationPNG_Next();
}

bool ImageFactory::decodeAnimationPNG_Buf(uint8_t* buf,size_t len) {
  try {
    if (animation_context_) {
      LogError(TAG) << "decodeAnimationBuf context error";
    }

    const char* data = (const char*)buf;
    auto loader = new uc::apng::loader<std::istringstream>(
        uc::apng::make_unique<std::istringstream>(std::string(data, len)));
    animation_context_ = (void*)loader;
    animation_file_name_.clear();
    animation_buf_ = buf;
    animation_buf_size_ = len;
    animation_loop_count_ = loader->num_plays();
  } catch (...) {
    LogError(TAG) << "APNG_buf error";
    return false;
  }
 return decodeAnimationPNG_Next();
}

bool ImageFactory::decodeAnimationPNG_Next() {
  try {
    uc::apng::loader<std::ifstream>* loader = (uc::apng::loader<std::ifstream>*)animation_context_;
    if (loader == nullptr) {
      bool retv = false;
      if (!animation_file_name_.empty()) {
        retv = decodeAnimationPNG_File(animation_file_name_);
      } else if (animation_buf_ != nullptr) {
        retv = decodeAnimationPNG_Buf(animation_buf_,animation_buf_size_);
      }
      return retv;
    }

    if (!loader->has_frame()) {
      animationPNG_CleanUp();
      return false;
    }
    auto frame = loader->next_frame();
    frame_duration_ = 1000 * frame.delay_num / frame.delay_den;
    if (buf_ == nullptr) {
      bpp_ = 32;
      width_ = frame.image.width();
      height_ = frame.image.height();
      buf_ = (uint8_t*)STBI_MALLOC(width_ * height_ * 4);
    }
    ::memcpy(buf_, frame.image.data(), width_ * height_ * 4);
  } catch (...) {
    LogError(TAG) << "APNG_next error";
    return false;
  }
  return true;
}

int64_t ImageFactory::getAnimationPNG_Delay() {
  return frame_duration_;
}

void ImageFactory::animationPNG_CleanUp() {
  try {
    uc::apng::loader<std::ifstream>* loader = (uc::apng::loader<std::ifstream>*)animation_context_;
    if (loader) {
      delete loader;
      animation_context_ = nullptr;
    }
    frame_duration_ = 0;
  } catch (...) {
    LogError(TAG) << "APNG_CleanUp() error";
  }
}

////////////////////////////////////////////////////////////////
// Animated WebP
////////////////////////////////////////////////////////////////
#include "webp/demux.h"

bool ImageFactory::isAnimatedWebP(uint8_t* buf, size_t buf_size) {
  WebPData webp_data;
    WebPDataInit(&webp_data);
    webp_data.bytes = buf;
    webp_data.size = buf_size;

  WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, NULL);
  if (dec == nullptr) {
    return false;
  }

  WebPAnimInfo anim_info;
  if (!WebPAnimDecoderGetInfo(dec, &anim_info)) {
    WebPAnimDecoderDelete(dec);
    return false;
  }

  bool animation = anim_info.frame_count > 1 ? true : false;
  WebPAnimDecoderDelete(dec);
  return animation;
}

bool ImageFactory::decodeAnimationWebP_File(const std::string& fileName) {
  FILE* fp = ::fopen(fileName.c_str(),"r");
  if (fp == nullptr) {
    return false;
  }

  ::fseek(fp,0,SEEK_END);
  size_t len = ::ftell(fp);
  ::fseek(fp,0,SEEK_SET);
  uint8_t* buf = (uint8_t*)::malloc(len);
  ::fread(buf,1,len,fp);
  ::fclose(fp);

  WebPData webp_data;
    WebPDataInit(&webp_data);
    webp_data.bytes = buf;
    webp_data.size = len;

  WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, NULL);
  if (dec == nullptr) {
    ::free(buf);
    return false;
  }

  WebPAnimInfo anim_info;
  if (!WebPAnimDecoderGetInfo(dec, &anim_info)) {
    WebPAnimDecoderDelete(dec);
    ::free(buf);
    return false;
  }
  width_ = anim_info.canvas_width;
  height_ = anim_info.canvas_height;
  bpp_ = 32;

  animation_context_ = (void*)dec;
  animation_file_name_ = fileName;
  animation_buf_ = buf;
  animation_buf_size_ = len;
  animation_loop_count_ = anim_info.loop_count;
  return decodeAnimationWebP_Next();
}

bool ImageFactory::decodeAnimationWebP_Buf(uint8_t* buf,size_t len) {
  WebPData webp_data;
    WebPDataInit(&webp_data);
    webp_data.bytes = buf;
    webp_data.size = len;

  WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, NULL);
  if (dec == nullptr) {
    return false;
  }

  WebPAnimInfo anim_info;
  if (!WebPAnimDecoderGetInfo(dec, &anim_info)) {
    WebPAnimDecoderDelete(dec);
    return false;
  }
  width_ = anim_info.canvas_width;
  height_ = anim_info.canvas_height;
  bpp_ = 32;

  animation_context_ = (void*)dec;
  animation_file_name_.clear();
  animation_buf_ = buf;
  animation_buf_size_ = len;
  animation_loop_count_ = anim_info.loop_count;
  return decodeAnimationWebP_Next();
}

bool ImageFactory::decodeAnimationWebP_Next() {
  WebPAnimDecoder* dec = (WebPAnimDecoder*)animation_context_;
  if (dec == nullptr) {
    bool retv = false;
    if (!animation_file_name_.empty()) {
      retv = decodeAnimationWebP_File(animation_file_name_);
    } else if (animation_buf_ != nullptr) {
      retv = decodeAnimationWebP_Buf(animation_buf_, animation_buf_size_);
    }
    return retv;
  }

  if (!WebPAnimDecoderHasMoreFrames(dec)) {
    animationWebP_CleanUp();
    return false;
  }

  int timestamp;
  uint8_t* frame = nullptr;
  WebPAnimDecoderGetNext(dec, &frame, &timestamp);
  frame_duration_ = timestamp - prev_frame_timestamp_;
  prev_frame_timestamp_ = timestamp;

  if (buf_ == nullptr) {
    buf_ = (uint8_t*)WebPMalloc(width_ * height_ * 4);
  }
  ::memcpy(buf_, frame, width_ * height_ * 4);
  return true;
}

int64_t ImageFactory::getAnimationWebP_Delay() {
  return frame_duration_;
}

void ImageFactory::animationWebP_CleanUp() {
  WebPAnimDecoder* dec = (WebPAnimDecoder*)animation_context_;
  if (dec) {
    WebPAnimDecoderDelete(dec);
    if (!animation_file_name_.empty()) {
      ::free(animation_buf_);
      animation_buf_ = nullptr;
    }
    animation_context_ = nullptr;
  }
  frame_duration_ = 0;
  prev_frame_timestamp_ = 0;
}

///// Premultiply
void ImageFactory::premultiply() {
  uint32_t* buf32 = (uint32_t*)buf_;
  uint32_t* buf_end = buf32;
  buf_end +=  width_*height_;

  do {
    uint8_t* buf8 = (uint8_t*)buf32;
    uint16_t alpha = buf8[3];
    if (alpha == 255) {
    } else if (alpha == 0) {
      *buf32 = 0;
    } else {
      alpha += 1;
      *buf8++ = (*buf8 * alpha) >> 8;
      *buf8++ = (*buf8 * alpha) >> 8;
      *buf8++ = (*buf8 * alpha) >> 8;
    }
  } while(++buf32 < buf_end);
}

///// STB resize
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#ifdef __ARM_NEON
#define STBIR_NEON
#endif
#include "third_party/stb/stb_image_resize2.h"

bool ImageFactory::fitScaleDown(const int width,const int height) {
  if (animation_context_) {
    LogWarn(TAG) << "Not support GIF resizing";
    return false;
  }

  if (width == 0 && height == 0) {
    if (width_ > MAX_WH || height_ > MAX_WH) {
      return fitScaleDown(MAX_WH, MAX_WH);
    }
    return false;
  }

  if (width >= width_ &&
      height >= height_) {
    return false;
  }

  // Scale
  double scale = 1.0f;
  double w = width_;
  double h = height_;
  if (width > 0 && height > 0) {
    double x_scale = width/w;
    double y_scale = height/h;
    scale = std::min(x_scale,y_scale);
  } else {
    scale = (width > 0) ? width/w : height/h;
  }
  if (width_ * height_ < SCALEDOWN_MIN_DIMENSION &&
      scale > 0.5) {
    return false;
  }

  // XYWH
  w = width_*scale;
  h = height_*scale;
  int w_dst = std::round(w);
  int h_dst = std::round(h);
  if (w_dst == 0 || h_dst == 0) {
    return false;
  }
  if (w_dst == width_ &&
    h_dst == height_) {
    return false;
  }

  int bpp = bpp_/8;
  uint8_t* buf = (uint8_t*)STBI_MALLOC(w_dst*h_dst*bpp);

  // Resize
  int64_t t = getCurTick();
  stbir_resize_uint8_linear(buf_, width_, height_, 0,
                            buf,  w_dst,  h_dst,   0,
                            bpp == 4 ? STBIR_RGBA : STBIR_RGB);

  LogDebug(TAG) << "fitScaleDown " << width_ << "," << height_ << " -> "
                << w_dst << "," << h_dst << " "
                << " by " << width << "x" << height << " "
                << bpp_ << " BPP " << "Scale: " << scale << " "
                << getDiffTick(t) << " msec";
  Dispose();
  width_ = w_dst;
  height_ = h_dst;
  buf_ = buf;
  return true;
}

bool ImageFactory::fitCover(const int width,const int height) {
  if (animation_context_) {
    LogWarn(TAG) << "Not support GIF resizing";
    return false;
  }
  if (width == 0 &&
      height == 0) {
    return false;
  }
  if (width == width_ &&
      height == height_) {
    return false;
  }

  // XY SCALE
  double x_scale = 1.0;
  double y_scale = 1.0;
  double w = width_;
  double h = height_;
  if (width > 0 && height > 0) {
    x_scale = width/w;
    y_scale = height/h;
  } else if (width > 0) {
    x_scale = width/w;
    y_scale = x_scale;
  } else if (height > 0) {
    y_scale = height/h;
    x_scale = y_scale;
  }

  // WH cut
  int w_cut = 0;
  int h_cut = 0;
  double max_scale = std::max(x_scale,y_scale);
  w = width_ * max_scale;
  h = height_ * max_scale;

  double w_diff = std::abs(w - width);
  double h_diff = std::abs(h - height);
  if (w_diff > h_diff) {
    w_cut = std::round(w_diff/max_scale);
    w_cut = (w_cut > 1) ? w_cut : 0;
  } else {
    h_cut = std::round(h_diff/max_scale);
    h_cut = (h_cut > 1) ? h_cut : 0;
  }

  // XYWH
  int w_src = width_ - w_cut;
  int h_src = height_ - h_cut;
  int w_dst = (width == 0) ? std::round(w_src*x_scale) : width;
  int h_dst = (height == 0) ? std::round(h_src*y_scale) : height;
  if (w_src == 0 || h_src == 0 ||
      w_dst == 0 || h_dst == 0) {
    return false;
  }

  int x = w_cut/2;
  int y = h_cut/2;
  if (x == 0 && y == 0 &&
      w_src == w_dst &&
      h_src == h_dst) {
    return false;
  }

  // Resize
  int bpp = bpp_/8;
  uint8_t* buf = (uint8_t*)STBI_MALLOC(w_dst*h_dst*bpp);

  int stride = width_*bpp;
  int64_t t = getCurTick();
  stbir_resize_uint8_linear(buf_ + x*bpp + y*stride,
                            w_src, h_src, stride,
                            buf,
                            w_dst, h_dst, 0,
                            bpp == 4 ? STBIR_RGBA : STBIR_RGB);

  LogDebug(TAG) << " src " << width_ << "x" << height_
               << " ->  " << w_src << "x" << h_src
               << " dst " << w_dst << "x" << h_dst
               << " by  " << width << "x" << height
               << " diff "<< w_diff << "x" << h_diff
               << " cut " << w_cut << "x" << h_cut
               << " " << bpp_ << " BPP "
               << getDiffTick(t) << " msec";
  Dispose();
  width_ = w_dst;
  height_ = h_dst;
  buf_ = buf;
  return true;
}

///// STB write
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb/stb_image_write.h"

bool ImageFactory::writeBMP(const std::string& path,
                            uint8_t* buf,int width,int height,int bpp) {
  int bytes = bpp/8;
  int ok = stbi_write_bmp(path.c_str(),width,height,bytes,buf);
  if (ok == 0) {
    LogError(TAG) << __func__ << "(" << path << ") error";
    return false;
  }
  return true;
}

bool ImageFactory::writePNG(const std::string& path,
                            uint8_t* buf,int width,int height,int bpp) {
  int bytes = bpp/8;
  int ok = stbi_write_png(path.c_str(),width,height,bytes,buf,width*bytes);
  if (ok == 0) {
    LogError(TAG) << __func__ << "(" << path << ") error";
    return false;
  }
  return true;
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

