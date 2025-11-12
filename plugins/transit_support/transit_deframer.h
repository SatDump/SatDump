#pragma once

#include <cstdint>
#include <vector>
#include <cmath>

// Modified version of SimpleDeframer for transit syncword system

// Transit 5B-5 appears to use a syncword of 11100010010xxxxx where x is 1 of 0 depending on the subframe, there are only 2 subframes in a frame

namespace transit
{
    class TransitDeframer
    {
    private:
        const int d_thresold;

        bool in_frame = false;
        bool synced = false;
        std::vector<uint8_t> current_frame;

        const uint16_t syncword_shared = 0xE240;
        const uint16_t sync_mask = 0xFFFF;
        const int frame_length = 32*8;
        const int syncword_length = 16;

        uint16_t asm_shifter = 0;

        uint8_t byte_shifter;
        int in_byte_buffer = 0;

        void push_bit(uint8_t bit)
        {
            byte_shifter = (byte_shifter << 1) | bit;
            in_byte_buffer++;
            if (in_byte_buffer == 8)
            {
                current_frame.push_back(byte_shifter);
                in_byte_buffer = 0;
            }
        }

        bool check_syncword(uint16_t buffer)
        {
            int sf1_errors = 0;
            int sf2_errors = 0;

            // shared part
            for(int i=syncword_length - 1; i >= 5; i--)
            {
                bool bit = (buffer >> i) & 0x1;
                bool correct_bit = (syncword_shared >> i) & 0x1;
                if(bit != correct_bit)
                {
                    sf1_errors++;
                    sf2_errors++;
                }
            }

            // unique part
            for(int i=syncword_length - 11 - 1; i >= 0; i--)
            {
                bool bit = (buffer >> i) & 0x1;
                if(bit)
                    sf1_errors++;
                else
                    sf2_errors++;
            }

            int errors = sf1_errors > sf2_errors ? sf2_errors : sf1_errors;

            return errors <= d_thresold;
        }

    public:
        TransitDeframer(int thresold)
            : d_thresold(thresold)
        {
            
        }

        // get deframer state 0: not synced, 1 = synced
        int getState()
        {
            return synced;
        }

        std::vector<std::vector<uint8_t>> work(int8_t *soft, int size)
        {
            std::vector<std::vector<uint8_t>> output_frames;

            for (int byten = 0; byten < size; byten++)
            {
                for (int i = 7; i >= 0; i--)
                {
                    uint8_t bit = ((soft)[byten * 8 + (7 - i)] > 0);
                    asm_shifter = (asm_shifter << 1 | bit) & sync_mask;

                    if (in_frame)
                    {
                        push_bit(bit);

                        if ((int)current_frame.size() * 8 == frame_length)
                        {
                            output_frames.push_back(current_frame);
                            in_frame = false;
                        }
                    }

                    if(check_syncword(asm_shifter))
                    {
                        if (in_frame)
                        {
                            // Fill up what we're missing
                            while ((int)current_frame.size() * 8 < frame_length)
                                push_bit(0);
                            output_frames.push_back(current_frame);
                        }

                        synced = true;
                        in_frame = true;
                        current_frame.clear();

                        // push ASM
                        for(int i=syncword_length - 1; i >= 0; i--)
                        {
                            uint8_t bit = (asm_shifter >> i) & 0x1;
                            push_bit(bit);
                        }
                    }
                    else
                    {
                        if(!in_frame)
                        {
                            synced = false;
                        }
                    }
                }
            }

            return output_frames;
        }
    };
};