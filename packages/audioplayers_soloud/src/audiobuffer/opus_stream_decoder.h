#ifndef OPUS_STREAM_DECODER_H
#define OPUS_STREAM_DECODER_H

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
// For Web include dirs downloaded from git for build
#include "../../xiph/opus/include/opus.h"
#include "../../xiph/ogg/include/ogg/ogg.h"
#else
#include <fstream>
#include <deque>
#include <opus/opus.h>
#include <ogg/ogg.h>
#endif

/// Wrapper class for Opus stream mDecoder
///
/// The supported mSampleRate for Opus format are 8, 12, 16, 24 amd 48 KHz.
/// The mChannels is the number of mChannels in the audio data. Only 1 or 2 allowed.
class OpusDecoderWrapper
{
public:
    OpusDecoderWrapper(int sampleRate, int channels)
        : mSampleRate(sampleRate), mChannels(channels), mPacketsInitialized(false)
    {
        int error;
        mDecoder = opus_decoder_create(mSampleRate, mChannels, &error);
        if (error != OPUS_OK)
        {
            throw std::runtime_error("Failed to create Opus mDecoder: " + std::string(opus_strerror(error)));
        }

        ogg_sync_init(&oy);
    }

    ~OpusDecoderWrapper()
    {
        if (mDecoder)
            opus_decoder_destroy(mDecoder);

        if (mPacketsInitialized)
        {
            ogg_stream_clear(&os);
        }
        ogg_sync_clear(&oy);
    }

    unsigned int decodeInfo(const unsigned char *inputData, size_t inputSize)
    {
        unsigned int sampleCount = 0;

        // Write data into ogg sync buffer
        char *buffer = ogg_sync_buffer(&oy, inputSize);
        memcpy(buffer, inputData, inputSize);
        ogg_sync_wrote(&oy, inputSize);

        // Read and process pages
        bool headerParsed = false;
        int packetCount = 0;
        unsigned int offset = 0;
        while (ogg_sync_pageout(&oy, &og) == 1)
        {
            if (!mPacketsInitialized)
            {
                ogg_stream_init(&os, ogg_page_serialno(&og));
                mPacketsInitialized = true;
            }

            if (ogg_stream_pagein(&os, &og) < 0)
            {
                throw std::runtime_error("Error reading Ogg page");
            }

            // Extract mPackets from page
            while (ogg_stream_packetout(&os, &op) == 1)
            {
                // Skip header mPackets (first 2 mPackets in Ogg Opus stream)
                if (!headerParsed)
                {
                    if (packetCount == 1)
                    {
                        // OpusTags packet
                        headerParsed = true;
                    }
                    packetCount++;
                    continue;
               }

               int frame_count = opus_packet_get_nb_frames(op.packet, op.bytes);
               int samples_per_frame = opus_packet_get_samples_per_frame(op.packet, mSampleRate);
               int count = frame_count * samples_per_frame;
               if (count < 0) {
                   continue;
               }

               mSamplesPerFrame = count;
               mOffsets.push_back(offset);
               mPackets.insert(mPackets.end(), op.packet, op.packet + op.bytes);

               offset += op.bytes;
               sampleCount += count;
            }
        }

        mOffsets.push_back(offset);
        return sampleCount;
    }

    std::vector<float> decodePacket(unsigned int sampleIndx, unsigned int samplesToRead)
    {
        std::vector<float> packetPcm;
        const int maxFrameSize = mSampleRate;
        std::vector<float> outputBuffer(maxFrameSize * mChannels);

        size_t indx = sampleIndx / mSamplesPerFrame;
        if (indx >= mPackets.size())
        {
            std::vector<float> packetPcm;
            return packetPcm;
        }

        unsigned char* base = mPackets.data();
        base += mOffsets[indx];
        size_t* offset = mOffsets.data();
        offset += indx;
        size_t* endOffset = mOffsets.data();
        endOffset += mOffsets.size() - 1;

        unsigned int readSamples = 0;
        while (true) {
            size_t size = offset[1] - offset[0];
            int samples = opus_decode_float(mDecoder,
                                            base,
                                            size,
                                            outputBuffer.data(),
                                            maxFrameSize,
                                            0);
            if (samples < 0)
            {
                std::cerr << "Warning: Failed to decode packet: " << opus_strerror(samples) << std::endl;
                return packetPcm; // Skip invalid packet instead of throwing
            }
            if (samples > 0)
            {
                packetPcm.insert(packetPcm.end(),
                                 outputBuffer.begin(),
                                 outputBuffer.begin() + samples * mChannels);
            }
            readSamples += samples;
            if (readSamples >= samplesToRead) {
              break;
            }

            base += size;
            offset++;
            if (endOffset <= offset)
            {
                break;
            }
        }
        return packetPcm;
    }

    unsigned int samplesPerFrame()
    {
        return mSamplesPerFrame;
    }

private:
    OpusDecoder *mDecoder = nullptr;
    int mSampleRate = 0;
    int mChannels = 0;

    // Ogg state variables
    ogg_sync_state oy;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    bool mPacketsInitialized = false;

    unsigned int mSamplesPerFrame = 0;
    std::vector<unsigned char> mPackets;
    std::vector<unsigned int> mOffsets;
};

#endif // OPUS_STREAM_DECODER_H
