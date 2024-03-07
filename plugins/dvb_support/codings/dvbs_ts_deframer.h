#pragma once

#include <cstdint>
#include <cstring>
#include "common/repack_bits_byte.h"

namespace deframing
{
    /*
    Simple deframer synchronizing TS frames.
    This will synchronize a packet of 8
    interleaved and randomized TS frames.

    The syncs are not interleaved nor randomized.

    TODO : IMPROVE!!!!!
    */
    class DVBS_TS_Deframer
    {
    public:
        const uint8_t TS_ASM = 0x47;
        const uint8_t TS_ASM_INV = 0xB8;
        const int TS_SIZE = 1632 * 8; // RS-Encoded

    private:
        // int d_state = STATE_NOSYNC; // Default state
        bool in_frame = false;      // Are we currently outpuing a frame?
        uint8_t shifter = 0;        // Shiter used to find the ASM
        bool bit_inversion = false; // Do we need to correct for bit inversion

        int bit_of_frame = 0;  // Bit of frame we're at
        uint8_t *frame_buffer; // Current WIP frame buffer

        int d_invalid_asm, d_good_asm; // Lock monitoring

        // Compare 2 8-bits values and return the difference count
        int compare_8(uint8_t v1, uint8_t v2)
        {
            int cor = 0;
            uint8_t diff = v1 ^ v2;
            for (; diff; cor++)
                diff &= diff - 1;
            return cor;
        }

        void write_bit(uint8_t b);
        void reset_frame();

        uint8_t *full_frame_shifter;
        uint8_t sync_bytes[8];

        RepackBitsByte repack;
        uint8_t *buf_bytes; // Repacked bytes buffer

    public:
        DVBS_TS_Deframer();
        ~DVBS_TS_Deframer();

        // Returns current state
        int getState();

        /*
        Actual deframing function. Takes unpacked bits in,
        returns frames as packed bytes in the output buffer.

        The return value is the frame count.
        */
        int work(uint8_t *input, int size, uint8_t *output);
    };
}