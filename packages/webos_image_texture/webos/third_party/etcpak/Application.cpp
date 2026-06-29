#include <future>
#include <stdio.h>
#include <limits>
#include <math.h>
#include <memory>
#include <string.h>
#include <iostream>
#include <sstream>

#include "bc7enc.h"
#include "Bitmap.hpp"
#include "BlockData.hpp"
#include "DataProvider.hpp"
#include "Debug.hpp"
#include "Error.hpp"
#include "System.hpp"
#include "TaskDispatch.hpp"
#include "Timing.hpp"
#include "TextureHeader.hpp"
#include "Application.hpp"
#include "Math.hpp"

#include "log.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "etcpak::"

static bool IsGoodBGR(const std::string& url,  const Bitmap& bmp, const Bitmap& out) {
  const float BAD = sq(50);
  const float BAD_MAX_RATIO = 0.0015;
  const size_t MIN_BLOCKS = 30000;
  const float MIN_PSNR = 30;
  const float MID_PSNR = 33;
  const float MAX_PSNR = 35;

  float mse = 0;
  size_t bad_cnt = 0;

  size_t w = bmp.Size().x;
  size_t h = bmp.Size().y;
  uint32_t* op1 = (uint32_t*)bmp.Data();
  uint32_t* op2 = (uint32_t*)out.Data();
  for (size_t i = 0; i < h; i += 4) {
    uint32_t* p1 = op1;
    uint32_t* p2 = op2;
    p1 += i*w;
    p2 += i*w;
    for (size_t j = 0; j < w; j += 4) {
      p1 += 4;
      p2 += 4;
      uint32_t c1 = *p1;
      uint32_t c2 = *p2;
      float err =  sq(( c1 & 0x000000FF ) - (( c2 & 0x00FF0000 ) >> 16));
      err +=  sq((( c1 & 0x0000FF00 ) >> 8) - ((c2 & 0x0000FF00) >> 8 ));
      err +=  sq((( c1 & 0x00FF0000 ) >> 16) -  (c2 & 0x000000FF));
      mse += err;
      if (err > BAD) {
        bad_cnt++;
      }
    }
  }

  mse *= 16;
  mse /= (w*h*3);

  float psnr = 20 * log10( 255 ) - 10 * log10( mse );
  size_t blocks = (w * h) /16;
  float bad_ratio = (float)bad_cnt/blocks;
  std::stringstream ss;
  ss << " blocks: " << blocks
     << " bads: " << bad_ratio * 100 << " %"
     << ", psnr: " << psnr
     << " " << url;

  if (bad_ratio < BAD_MAX_RATIO && psnr > MIN_PSNR) {
    LogInfo(TAG) << "OK by not-bad pixels," << ss.str();
    return true;
  }

  if (blocks > MIN_BLOCKS && psnr > MID_PSNR) {
    LogInfo(TAG) << "OK by num of blocks," << ss.str();
    return true;
  }

  if (psnr > MAX_PSNR) {
    LogInfo(TAG) << "OK by PSNR," << ss.str();
    return true;
  }

  LogInfo(TAG) << "Fail by BAD," << ss.str();
  return false;
}

bool etcpak_encoder(const std::string& url, bool hasAlpha, const char *input, const char *output)
{
    bool mipmap = false;
    bool dither = false;
    bool dxtc = false;
    bool linearize = true;
    bool useHeuristics = true;
    auto codec = CodecType::Etc2_RGB;
    auto header = BlockData::Format::Pvr;
    bool good = false;

    enum Options
    {
        OptLinear,
        OptNoHeuristics
    };

    const bool bgr = !(codec == CodecType::Bc1 || codec == CodecType::Bc3 || codec == CodecType::Bc4 || codec == CodecType::Bc5 || codec == CodecType::Bc7);
    const bool rgba = (codec == CodecType::Etc2_RGBA || codec == CodecType::Bc3 || codec == CodecType::Bc7 || hasAlpha);
    bc7enc_compress_block_params bc7params;
    if (codec == CodecType::Bc7)
    {
        bc7enc_compress_block_init();
        bc7enc_compress_block_params_init(&bc7params);
    }

    {
        DataProvider dp(input, mipmap, !dxtc, linearize);
        auto num = dp.NumberOfParts();
        if (rgba && dp.Alpha())
        {
            codec = CodecType::Etc2_RGBA;
        }
        else
        {
            codec = CodecType::Etc2_RGB;
        }
        auto bd = std::make_shared<BlockData>(output, dp.Size(), mipmap, codec, header);
        for (int i = 0; i < num; i++)
        {
            auto part = dp.NextPart();

            if (rgba)
            {
                bd->ProcessRGBA(part.src, part.width / 4 * part.lines, part.offset, part.width, useHeuristics, &bc7params);
            }
            else
            {
                bd->Process(part.src, part.width / 4 * part.lines, part.offset, part.width, dither, useHeuristics);
            }
        }

        {
          auto out = bd->Decode();
          good = IsGoodBGR(url, dp.ImageData(), *out);
        }
    }
    return good;
}

void etcpak_decoder(const char *input, const char *output)
{
    {
        auto bd = std::make_shared<BlockData>(input);
        auto out = bd->Decode();
        out->Write(output);
    }
}
