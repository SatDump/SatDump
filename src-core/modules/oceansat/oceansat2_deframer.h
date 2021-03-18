#pragma once

/*
    An arbitrary deframer
*/

#include <vector>
#include <array>
#include <cstdint>

namespace oceansat
{
    class Oceansat2Deframer
    {
    private:
        // Main shifter
        uint64_t shifter;
        // Small function to push a bit into the frame
        void pushBit(uint8_t bit);
        // Framing variables
        uint8_t byteBuffer;
        bool writeFrame;
        int wroteBits, outputBits;
        std::vector<uint8_t> frameBuffer;
        const int frameSize = 737760;

    public:
        Oceansat2Deframer();
        std::vector<std::vector<uint8_t>> work(uint8_t *data, int length);
    };
} // namespace oceansat