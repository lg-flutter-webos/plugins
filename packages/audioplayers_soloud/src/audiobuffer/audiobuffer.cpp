#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mutex>

#include "audiobuffer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace SoLoud
{
    std::recursive_mutex buffer_lock_mutex;

    BufferStreamInstance::BufferStreamInstance(BufferStream *aParent)
    {
        mParent = aParent;
        mOffset = 0;
    }

    BufferStreamInstance::~BufferStreamInstance()
    {
    }

    unsigned int BufferStreamInstance::getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize)
    {
        std::lock_guard<std::recursive_mutex> lock(buffer_lock_mutex);
        unsigned int sampleCount = mParent->mSampleCount;
        int samplesToRead = mOffset + aSamplesToRead > sampleCount
                                ? sampleCount - mOffset
                                : aSamplesToRead;
        if (samplesToRead <= 0)
        {
            memset(aBuffer, 0, sizeof(float) * aSamplesToRead);
            updatePosition();
            return 0;
        }

        // fill aBuffer
        if (samplesToRead != aSamplesToRead)
        {
            memset(aBuffer, 0, sizeof(float) * aSamplesToRead * mChannels);
        }

        unsigned int bufferSize = mParent->mBuffer.getFloatsBufferSize()/mChannels;
        if (samplesToRead > bufferSize) {
            auto pcm = mParent->decode(mOffset + bufferSize, samplesToRead - bufferSize);
            mParent->mBuffer.addData(BufferType::OPUS, pcm.data(), pcm.size());
        }

        float *buffer = reinterpret_cast<float *>(mParent->mBuffer.buffer.data());
        if (mChannels == 1)
        {
            memcpy(aBuffer, buffer, sizeof(float) * samplesToRead);
        }
        else
        {
            unsigned int i, j;
            for (j = 0; j < mChannels; j++)
            {
                for (i = 0; i < samplesToRead; i++)
                {
                    aBuffer[j * samplesToRead + i] = buffer[i * mChannels + j];
                }
            }
        }

        // erase mBuffer
        mParent->mBuffer.removeData(samplesToRead * mChannels * sizeof(float));

        // Update stream position
        mOffset += samplesToRead;
        updatePosition();
        return samplesToRead;
    }

    result BufferStreamInstance::seek(double aSeconds, float *mScratch, unsigned int mScratchSize)
    {
        std::lock_guard<std::recursive_mutex> lock(buffer_lock_mutex);
        if (mParent->mSampleCount == 0 || mParent->mSamplesPerFrame == 0)
        {
            return SO_NO_ERROR;
        }

        unsigned int samples = aSeconds * mBaseSamplerate;
        if (samples > mParent->mSampleCount)
        {
            samples = mParent->mSampleCount;
        }
        samples = samples / mParent->mSamplesPerFrame * mParent->mSamplesPerFrame;

        mOffset = samples;
        updatePosition();
        mParent->mBuffer.clear();
        return SO_NO_ERROR;
    }

    result BufferStreamInstance::rewind()
    {
        std::lock_guard<std::recursive_mutex> lock(buffer_lock_mutex);

        mOffset = 0;
        updatePosition();
        mParent->mBuffer.clear();
        return 0;
    }

    bool BufferStreamInstance::hasEnded()
    {
        if (mParent->mDataIsEnded &&
            mOffset >= mParent->mSampleCount)
        {
            return 1;
        }
        return 0;
    }

    void BufferStreamInstance::updatePosition()
    {
        mStreamPosition = (float)mOffset / mBaseSamplerate;
        mStreamTime = mStreamPosition;
    }

    // //////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////

    BufferStream::BufferStream() {}

    BufferStream::~BufferStream()
    {
        stop();
    }

    PlayerErrors BufferStream::setBufferStream(
        Player *aPlayer,
        ActiveSound *aParent,
        unsigned int maxBufferSize,
        BufferingType bufferingType,
        SoLoud::time bufferingTimeNeeds,
        PCMformat pcmFormat,
        dartOnBufferingCallback_t onBufferingCallback)
    {
        mParent = aParent;
        mPCMformat.sampleRate = pcmFormat.sampleRate;
        mPCMformat.channels = pcmFormat.channels;
        mPCMformat.bytesPerSample = pcmFormat.bytesPerSample;
        mPCMformat.dataType = pcmFormat.dataType;
        mChannels = pcmFormat.channels;
        mBaseSamplerate = (float)pcmFormat.sampleRate;

        mBuffer.clear();
        mBuffer.setSizeInBytes(maxBufferSize);
        mBuffer.setBufferType(bufferingType);

        mSampleCount = 0;
        mSamplesPerFrame = 0;
        mDataIsEnded = false;
        mEncBuffer.clear();
        mDecoder = nullptr;

        try
        {
            mDecoder = std::make_unique<OpusDecoderWrapper>(
                pcmFormat.sampleRate, pcmFormat.channels);
        }
        catch (const std::exception &e)
        {
            return PlayerErrors::failedToCreateOpusDecoder;
        }

        return PlayerErrors::noError;
    }

    PlayerErrors BufferStream::addData(const void *aData, unsigned int aDataLen, bool forceAdd)
    {
        if (mDataIsEnded)
        {
            return PlayerErrors::streamEndedAlready;
        }

        mEncBuffer.insert(mEncBuffer.end(),
            static_cast<const unsigned char *>(aData),
            static_cast<const unsigned char *>(aData) + aDataLen);
    return PlayerErrors::noError;
    }

    void BufferStream::setDataIsEnded()
    {
        mSampleCount = mDecoder.get()->decodeInfo(
                           mEncBuffer.data(),
                           mEncBuffer.size());
        mSamplesPerFrame = mDecoder.get()->samplesPerFrame();
        mEncBuffer.clear();
        mDataIsEnded = true;
    }

    std::vector<float> BufferStream::decode(unsigned int offset, unsigned int samplesToRead) {
        return mDecoder.get()->decodePacket(offset, samplesToRead);
    }

    BufferingType BufferStream::getBufferingType()
    {
        return mBuffer.bufferingType;
    }

    AudioSourceInstance *BufferStream::createInstance()
    {
        return new BufferStreamInstance(this);
    }

    double BufferStream::getLength()
    {
        if (mBaseSamplerate == 0)
            return 0;
        return mSampleCount / mBaseSamplerate;
    }
};
