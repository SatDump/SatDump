#pragma once

/*
    An arbitrary deframer
*/

#include <vector>
#include <array>
#include <cstdint>

namespace fengyun
{
    namespace windrad
    {
        class WindRADDeframer1
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

        public:
            WindRADDeframer1();
            std::vector<std::vector<uint8_t>> work(uint8_t *input, int size);
        };

        class WindRADDeframer2
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

        public:
            WindRADDeframer2();
            std::vector<std::vector<uint8_t>> work(uint8_t *input, int size);
        };
    } // namespace virr
} // namespace fengyun