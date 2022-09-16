#pragma once
#include <cstdint>

/*
    CCSDS Values to use throughout the whole program
*/

namespace meteor
{
    const int CADU_SIZE = 1024;
    const int CADU_ASM_SIZE = 4;
    const uint32_t CADU_ASM = 0x1ACFFC1D;
    const uint32_t CADU_ASM_INV = 0xE53003E2;
    const uint8_t CADU_ASM_1 = 0x1A;
    const uint8_t CADU_ASM_2 = 0xCF;
    const uint8_t CADU_ASM_3 = 0xFC;
    const uint8_t CADU_ASM_4 = 0x1D;
}