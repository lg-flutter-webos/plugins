#pragma once

#ifndef _AUDIOBUFFER_H
#define _AUDIOBUFFER_H

#include <stdio.h>
#include <atomic>
#include <chrono>
#include "soloud.h"
#include "../player.h"
#include "../enums.h"
#include "../active_sound.h"
#include "buffer.h"
#include "opus_stream_decoder.h"

class Player;

namespace SoLoud
{
  class BufferStream;

  class BufferStreamInstance : public AudioSourceInstance
  {
    BufferStream *mParent;
    unsigned int mOffset;

  public:
    BufferStreamInstance(BufferStream *aParent);
    virtual unsigned int getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize);
    virtual result seek(double aSeconds, float *mScratch, unsigned int mScratchSize);
    virtual result rewind();
    virtual bool hasEnded();
    virtual ~BufferStreamInstance();

  private:
    void updatePosition();
  };

  class BufferStream : public AudioSource
  {
  public:
    // Used to access the AudioSource this stream belongs to
    ActiveSound* mParent = nullptr;
    PCMformat mPCMformat;

    unsigned int mSampleCount = 0;
    unsigned int mSamplesPerFrame = 0;
    bool mDataIsEnded = false;
    std::vector<unsigned char> mEncBuffer;
    Buffer mBuffer;
    std::unique_ptr<OpusDecoderWrapper> mDecoder;

    BufferStream();

    virtual ~BufferStream();

    PlayerErrors setBufferStream(
        Player *aPlayer,
        ActiveSound *aParent,
        unsigned int maxBufferSize = 1024 * 1024 * 100, // 100 Mbytes
        BufferingType bufferingType = BufferingType::PRESERVED,
        time bufferingTimeNeeds = 2.0f, // 2 seconds of data to wait
        PCMformat pcmFormat = {44100, 2, 2, PCM_S16LE},
        dartOnBufferingCallback_t onBufferingCallback = nullptr);

    void resetBuffer() {}

    void setDataIsEnded();

    PlayerErrors addData(const void *aData, unsigned int numSamples, bool forceAdd = false);

    BufferingType getBufferingType();

    virtual AudioSourceInstance *createInstance();

    time getLength();

    std::vector<float> decode(unsigned int offset, unsigned int samplesToRead);
  };
};

#endif // _AUDIOBUFFER_H
