#pragma once

/*
    An arbitrary deframer
*/

#include <vector>
#include <array>
#include <cstdint>

class VIRRDeframer
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
    VIRRDeframer();
    std::vector<std::vector<uint8_t>> work(std::vector<uint8_t> &data);
};