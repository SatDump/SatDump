#include "bpsk_ccsds_deframer.h"
#include <cstdint>
#include <cstring>

namespace deframing
{
    BPSK_CCSDS_Deframer::BPSK_CCSDS_Deframer(int cadu_size, uint32_t sync) : CADU_ASM(sync),
                                                                             CADU_ASM_INV(sync ^ 0xFFFFFFFF),
                                                                             CADU_SIZE(cadu_size)
    {
        frame_buffer = new uint8_t[CADU_SIZE + CADU_PADDING];
    }

    BPSK_CCSDS_Deframer::~BPSK_CCSDS_Deframer()
    {
        delete[] frame_buffer;
    }

    int BPSK_CCSDS_Deframer::getState()
    {
        return d_state;
    }

    int BPSK_CCSDS_Deframer::work(uint8_t *input, int size, uint8_t *output)
    {
        int frame_count = 0;

        for (int ibit = 0; ibit < size; ibit++) // We work bit-per-bit
        {
            shifter = shifter << 1 | input[ibit];

            if (in_frame)
            {
                write_bit(input[ibit] ^ bit_inversion);

                if (bit_of_frame == CADU_SIZE) // Write frame out
                {
                    memcpy(&output[frame_count * ((CADU_SIZE + CADU_PADDING) / 8)], frame_buffer, (CADU_SIZE + CADU_PADDING) / 8);
                    frame_count++;
                }
                else if (bit_of_frame == CADU_SIZE + CADU_ASM_SIZE - 1) // Skip to the next ASM
                {
                    in_frame = false;
                }

                continue;
            }

            if (d_state == STATE_NOSYNC)
            {
                if (shifter == CADU_ASM) //(compare_32(shifter, CADU_ASM) < d_state)
                {
                    bit_inversion = false;
                    reset_frame();
                    in_frame = true;
                    d_state = STATE_SYNCING;
                    d_good_asm = d_invalid_asm = 0;
                }
                else if (shifter == CADU_ASM_INV) //(compare_32(shifter, CADU_ASM_INV) < d_state)
                {
                    bit_inversion = true;
                    reset_frame();
                    in_frame = true;
                    d_state = STATE_SYNCING;
                    d_good_asm = d_invalid_asm = 0;
                }
            }
            else if (d_state == STATE_SYNCING)
            {
                if (compare_32(shifter, bit_inversion ? CADU_ASM_INV : CADU_ASM) < d_state)
                {
                    reset_frame();
                    in_frame = true;
                    d_invalid_asm = 0;
                    d_good_asm++;

                    if (d_good_asm > 10)
                        d_state = STATE_SYNCED;
                }
                else
                {
                    d_invalid_asm++;
                    d_good_asm = 0;

                    if (d_invalid_asm > 2)
                    {
                        d_state = STATE_NOSYNC;
                    }
                }
            }
            else if (d_state == STATE_SYNCED)
            {
                if (compare_32(shifter, bit_inversion ? CADU_ASM_INV : CADU_ASM) < d_state)
                {
                    reset_frame();
                    in_frame = true;
                }
                else
                {
                    d_good_asm = d_invalid_asm = 0;
                    d_state = STATE_NOSYNC; // Reset to hard NOSYNC, so we correct for a possible new inversion state
                }
            }
        }

        return frame_count;
    }

    void BPSK_CCSDS_Deframer::write_bit(uint8_t b)
    {
        frame_buffer[bit_of_frame / 8] = frame_buffer[bit_of_frame / 8] << 1 | b;
        bit_of_frame++;
    }

    void BPSK_CCSDS_Deframer::reset_frame()
    {
        memset(frame_buffer, 0, (CADU_SIZE + CADU_PADDING) / 8);
        frame_buffer[0] = CADU_ASM >> 24;
        frame_buffer[1] = CADU_ASM >> 16;
        frame_buffer[2] = CADU_ASM >> 8;
        frame_buffer[3] = CADU_ASM >> 0;
        bit_of_frame = CADU_ASM_SIZE;
    }
}