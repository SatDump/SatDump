#pragma once

#include <cstdint>
#include <cstring>

namespace deframing
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
    class BPSK_CCSDS_Deframer
    {
    public:
        const uint32_t CADU_ASM;      // ASM
        const uint32_t CADU_ASM_INV;  // Inverted ASM
        const int CADU_ASM_SIZE = 32; // ASM bit length
        const int CADU_SIZE;          // CADU bit length, with ASM

        // There are left writable, so they can be customized if required
        int STATE_NOSYNC = 2;  // NOSYNC state, searching for a sync with very low tolerance (bit-level search)
        int STATE_SYNCING = 6; // SYNCING state. We found a lock, but can't trust it yet (bit-level search)
        int STATE_SYNCED = 12; // SYNCED state. We found a lock and confirmed it, skipping bit-level search and with high tolerance.

    private:
        int d_state = STATE_NOSYNC; // Default state
        bool in_frame = false;      // Are we currently outpuing a frame?
        uint32_t shifter = 0;       // Shiter used to find the ASM
        bool bit_inversion = false; // Do we need to correct for bit inversion

        int bit_of_frame = 0;  // Bit of frame we're at
        uint8_t *frame_buffer; // Current WIP frame buffer

        int d_invalid_asm, d_good_asm; // Lock monitoring

        // Compare 2 32-bits values and return the difference count
        int compare_32(uint32_t v1, uint32_t v2)
        {
            int cor = 0;
            uint32_t diff = v1 ^ v2;
            for (; diff; cor++)
                diff &= diff - 1;
            return cor;
        }

        void write_bit(uint8_t b);
        void reset_frame();

    public:
        BPSK_CCSDS_Deframer(int cadu_size = 8192, uint32_t sync = 0x1ACFFC1D);
        ~BPSK_CCSDS_Deframer();

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