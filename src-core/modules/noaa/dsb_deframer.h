#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include "dsb.h"

namespace noaa
{
    class DSBDeframer
    {
    private:
        // Main shifter used to locate sync words
        uint16_t shifter;
        // Bit inversion?
        bool bit_inversion;
        // Sync machine state
        int state;
        // Write a frame?
        bool writeFrame;
        // Values used to output a found frame
        int wroteBits, wroteBytes;
        uint8_t outBuffer;
        int skip, errors, good, sep_errors, state_2_bits_count;
        // Output Frame buffer
        std::array<uint8_t, FRAME_SIZE> frameBuffer;
        // Found frame count
        int numFrames;

    public:
        DSBDeframer();
        // Get found frame count
        int getFrameCount();
        // Return state
        int getState();
        // Perform deframing
        std::vector<std::array<uint8_t, FRAME_SIZE>> work(uint8_t *input, int size);
    };
} // namespace noaa