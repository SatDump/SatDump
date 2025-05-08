#pragma once

/*
    An arbitrary deframer
*/

#include <cstdint>
#include <vector>

namespace goes
{
    namespace gvar
    {
        class GVARDeframer
        {
        private:
            const int SYNC_SIZE = 64;
            const uint64_t ASM_SYNC = 0b0001101111100111110100000001111110111111100000001111111111111110;
            const int FRAME_SIZE = 262288;

        private:
            // Main shifter
            uint64_t shifter = 0;
            // Small function to push a bit into the frame
            void pushBit(uint8_t bit);
            // Framing variables
            uint8_t byteBuffer;
            bool writeFrame;
            int wroteBits, outputBits;
            std::vector<uint8_t> frameBuffer;

            // PN Stuff
            uint64_t pn_shifter = 0;
            int pn_right_bit_counter = 0;

        public:
            GVARDeframer();
            std::vector<std::vector<uint8_t>> work(uint8_t *data, int len);
        };
    } // namespace gvar
} // namespace goes