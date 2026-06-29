#include <filesystem>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <glib.h>

#include "wave_generator.h"
#include "log.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "WaveGenerator::"

////////////////////////////////////////////////////////////////////////////////
#define WAVE_FORMAT_UNKNOWN      0X0000;
#define WAVE_FORMAT_PCM          0X0001;
#define WAVE_FORMAT_MS_ADPCM     0X0002;
#define WAVE_FORMAT_IEEE_FLOAT   0X0003;
#define WAVE_FORMAT_ALAW         0X0006;
#define WAVE_FORMAT_MULAW        0X0007;
#define WAVE_FORMAT_IMA_ADPCM    0X0011;
#define WAVE_FORMAT_YAMAHA_ADPCM 0X0016;
#define WAVE_FORMAT_GSM          0X0031;
#define WAVE_FORMAT_ITU_ADPCM    0X0040;
#define WAVE_FORMAT_MPEG         0X0050;
#define WAVE_FORMAT_EXTENSIBLE   0XFFFE;

typedef struct
{
  uint8_t ChunkID[4];    // Contains the letters "RIFF" in ASCII form
  uint32_t ChunkSize;    // This is the size of the rest of the chunk following this number
  uint8_t Format[4];     // Contains the letters "WAVE" in ASCII form
} RIFF;

//-------------------------------------------
// [Channel]
// - streo     : [left][right]
// - 3 channel : [left][right][center]
// - quad      : [front left][front right][rear left][reat right]
// - 4 channel : [left][center][right][surround]
// - 6 channel : [left center][left][center][right center][right][surround]
//-------------------------------------------
typedef struct
{
  uint8_t  ChunkID[4];    // Contains the letters "fmt " in ASCII form
  uint32_t ChunkSize;     // 16 for PCM.  This is the size of the rest of the Subchunk which follows this number.
  uint16_t AudioFormat;   // PCM = 1
  uint16_t NumChannels;   // Mono = 1, Stereo = 2, etc.
  uint32_t SampleRate;    // 8000, 44100, etc.
  uint32_t AvgByteRate;   // SampleRate * NumChannels * BitsPerSample/8
  uint16_t BlockAlign;    // NumChannels * BitsPerSample/8
  uint16_t BitPerSample;  // 8 bits = 8, 16 bits = 16, etc
} FMT;

typedef struct
{
  char ChunkID[4];        // Contains the letters "data" in ASCII form
  uint32_t ChunkSize;     // NumSamples * NumChannels * BitsPerSample/8
} DATA;

typedef struct
{
  RIFF Riff;
  FMT  Fmt;
  DATA Data;
} WAVE_HEADER;

WaveGenerator::WaveGenerator() {
  const double DURATION = 0.1;
  const uint32_t SAMPLE_RATE = 44100;
  const uint32_t CHANNEL = 1;
  const uint32_t BIT_RATE = 16;
  uint32_t HEADER_SIZE = sizeof(WAVE_HEADER);
  uint32_t PCM_SIZE = DURATION * SAMPLE_RATE * CHANNEL * BIT_RATE / 8;

  auto buf = std::make_unique<uint8_t[]>(HEADER_SIZE + PCM_SIZE);
  ::memset(buf.get(), 0, HEADER_SIZE + PCM_SIZE);

  //..........................................
  WAVE_HEADER header;
  ::memcpy(header.Riff.ChunkID, "RIFF", 4);
  header.Riff.ChunkSize = PCM_SIZE + 36;
  ::memcpy(header.Riff.Format, "WAVE", 4);

  ::memcpy(header.Fmt.ChunkID, "fmt ", 4);
  header.Fmt.ChunkSize = 0x10;
  header.Fmt.AudioFormat = WAVE_FORMAT_PCM;
  header.Fmt.NumChannels = CHANNEL;
  header.Fmt.SampleRate = SAMPLE_RATE;
  header.Fmt.AvgByteRate = SAMPLE_RATE * CHANNEL * BIT_RATE / 8;
  header.Fmt.BlockAlign = CHANNEL * BIT_RATE / 8;
  header.Fmt.BitPerSample = BIT_RATE;

  ::memcpy(header.Data.ChunkID, "data", 4);
  header.Data.ChunkSize = PCM_SIZE;
  //...........................................

  ::memcpy(buf.get(), &header, HEADER_SIZE);
  gchar* gbase64 = g_base64_encode(buf.get(), HEADER_SIZE + PCM_SIZE);

  url_ = "data:audio/x-wav;base64,";
  url_ += gbase64;
  g_free(gbase64);
}

WaveGenerator::~WaveGenerator() {

}

std::string WaveGenerator::GetUrl() {
  return url_;
}
