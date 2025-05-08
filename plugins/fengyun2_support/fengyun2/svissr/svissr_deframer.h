#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace fengyun_svissr
{
    class SVISSRDeframer
    {
    private:
        // Main shifter
        uint64_t shifter;
        int goodBits;
        // Small function to push a bit into the frame
        void pushBit(uint8_t bit);
        // Framing variables
        uint8_t byteBuffer;
        bool writeFrame;
        int wroteBits, outputBits;
        std::vector<uint8_t> frameBuffer;

        // PN Stuff
        uint64_t pn_shifter;
        int pn_right_bit_counter = 0;

    public:
        SVISSRDeframer();
        std::vector<std::vector<uint8_t>> work(uint8_t *data, int len);
    };
} // namespace fengyun_svissr