#pragma once

/*
    A CADU Deframer
*/

#include <vector>
#include <array>
#include <cstdint>

namespace ccsds
{
    namespace ccsds_1_0_proba
    {
        /*
    CCSDS Values to use throughout the whole program
*/

#define CADU_SIZE 1279
#define CADU_ASM_SIZE 4
#define CADU_ASM 0x1ACFFC1D
#define CADU_ASM_INV 0xE53003E2
#define CADU_ASM_1 0x1A
#define CADU_ASM_2 0xCF
#define CADU_ASM_3 0xFC
#define CADU_ASM_4 0x1D

        class CADUDeframer
        {
        private:
            // Main shifter used to locate sync words
            uint32_t shifter;
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
            std::array<uint8_t, CADU_SIZE> frameBuffer;
            // Found frame count
            int numFrames;

        public:
            CADUDeframer();
            // Get found frame count
            int getFrameCount();
            // Return state
            int getState();
            // Perform deframing
            std::vector<std::array<uint8_t, CADU_SIZE>> work(uint8_t *input, size_t size);
        };
    }
}