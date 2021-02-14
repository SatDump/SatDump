#pragma once

/*
    A CADU Deframer
*/

#include <vector>
#include <array>
#include <cstdint>
#include "ccsds.h"

class CADUDeframer
{
private:
    // Main shifter used to locate sync words
    uint32_t shifter;
    // Bit inversion?
    bool bit_inversion;
    // Sync machine state
    int state;
    // Derandomization values
    uint8_t d_rantab[CADU_SIZE];
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