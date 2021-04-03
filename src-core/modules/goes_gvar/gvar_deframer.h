#pragma once

/*
    An arbitrary deframer
*/

#include <vector>
#include <array>
#include <cstdint>

namespace goes_gvar
{
    template <typename SYNC_T, int SYNC_SIZE, int FRAME_SIZE, SYNC_T ASM_SYNC>
    class GVARDeframer
    {
    private:
        // Main shifter
        SYNC_T shifter;
        // Small function to push a bit into the frame
        void pushBit(uint8_t bit);
        // Framing variables
        uint8_t byteBuffer;
        bool writeFrame;
        int wroteBits, outputBits;
        std::vector<uint8_t> frameBuffer;

    public:
        GVARDeframer();
        std::vector<std::vector<uint8_t>> work(uint8_t *data, int len);
    };
}