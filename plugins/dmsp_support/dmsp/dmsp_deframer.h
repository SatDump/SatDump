#pragma once

#include <cstdint>
#include <cstring>

namespace dmsp
{
    /*
    Simple deframer synchronizing fixed frame length frames
    against a 32-bit ASM, that may be bit-inverted.

    This is mostly designed for CCSDS-compliant purposes,
    utilizing the 0x1acffc1d ASM, hence CCSDS terminology
    is used all the way.

    It takes unpacked bits as input, mostly meant for either
    raw BPSK symbols or Viterbi-decoded bits, which will already
    have phase ambiguities corrected.

    Now of course, this will also work with any similar protocol
    utilizing a 32-bits ASM sequence, hence it is left configurable.
    */
    class DMSP_Deframer
    {
    public:
        const uint16_t FRM_ASM = 0b1010110011111;               // ASM
        const uint16_t FRM_ASM_INV = FRM_ASM ^ 0b1111111111111; // Inverted ASM
        const int FRM_ASM_SIZE = 13;                            // ASM bit length
        const int FRM_SIZE;                                     // CADU bit length, with ASM
        int FRM_PADDING = 0;                                    // Optional frame padding

        // There are left writable, so they can be customized if required
        int STATE_NOSYNC = 0;  // NOSYNC state, searching for a sync with very low tolerance (bit-level search)
        int STATE_SYNCING = 2; // SYNCING state. We found a lock, but can't trust it yet (bit-level search)
        int STATE_SYNCED = 6;  // SYNCED state. We found a lock and confirmed it, skipping bit-level search and with high tolerance.

    private:
        int d_state = STATE_NOSYNC; // Default state
        bool in_frame = false;      // Are we currently outpuing a frame?
        uint32_t shifter = 0;       // Shiter used to find the ASM
        bool bit_inversion = false; // Do we need to correct for bit inversion

        int bit_of_frame = 0;  // Bit of frame we're at
        uint8_t *frame_buffer; // Current WIP frame buffer

        int d_invalid_asm, d_good_asm; // Lock monitoring

        // Compare 2 16-bits values and return the difference count
        int compare_16(uint16_t v1, uint16_t v2)
        {
            int cor = 0;
            uint16_t diff = v1 ^ v2;
            for (; diff; cor++)
                diff &= diff - 1;
            return cor;
        }

        void write_bit(uint8_t b);
        void reset_frame();

    public:
        DMSP_Deframer(int frm_size, int padding);
        ~DMSP_Deframer();

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