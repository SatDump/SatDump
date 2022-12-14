#include "dsb_deframer.h"
#include <cstdint>
#include <cstring>

namespace noaa
{
    DSB_Deframer::DSB_Deframer(int cadu_size, int padding) : FRM_SIZE(cadu_size),
                                                             FRM_PADDING(padding)
    {
        frame_buffer = new uint8_t[FRM_SIZE + FRM_PADDING];
    }

    DSB_Deframer::~DSB_Deframer()
    {
        delete[] frame_buffer;
    }

    int DSB_Deframer::getState()
    {
        return d_state;
    }

    int DSB_Deframer::work(int8_t *input, int size, uint8_t *output)
    {
        int frame_count = 0;

        for (int ibit = 0; ibit < size; ibit++) // We work bit-per-bit
        {
            shifter = (shifter << 1 | (input[ibit] > 0)) & 0xFFFF;

            if (in_frame)
            {
                write_bit((input[ibit] > 0) ^ bit_inversion);

                if (bit_of_frame == FRM_SIZE) // Write frame out
                {
                    memcpy(&output[frame_count * ((FRM_SIZE + FRM_PADDING) / 8)], frame_buffer, (FRM_SIZE + FRM_PADDING) / 8);
                    frame_count++;
                }
                else if (bit_of_frame == FRM_SIZE + FRM_ASM_SIZE - 1) // Skip to the next ASM
                {
                    in_frame = false;
                }

                continue;
            }

            if (d_state == STATE_NOSYNC)
            {
                if (shifter == FRM_ASM)
                {
                    bit_inversion = false;
                    reset_frame();
                    in_frame = true;
                    d_state = STATE_SYNCING;
                    d_good_asm = d_invalid_asm = 0;
                }
                else if (shifter == FRM_ASM_INV)
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
                if (compare_16(shifter, bit_inversion ? FRM_ASM_INV : FRM_ASM) < d_state)
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
                if (compare_16(shifter, bit_inversion ? FRM_ASM_INV : FRM_ASM) < d_state)
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

    void DSB_Deframer::write_bit(uint8_t b)
    {
        frame_buffer[bit_of_frame / 8] = frame_buffer[bit_of_frame / 8] << 1 | b;
        bit_of_frame++;
    }

    void DSB_Deframer::reset_frame()
    {
        memset(frame_buffer, 0, (FRM_SIZE + FRM_PADDING) / 8);
        bit_of_frame = 0;
        for (int i = 15; i >= 0; i--)
            write_bit((FRM_ASM >> i) & 1);
    }
}