#pragma once

#include <bitset>
#include <cstdint>
#include <cstring>
#include <vector>

namespace fengyun_svissr
{
    class PNSync
    {
    public:
        const int BUFFER_SIZE;
        const int LOCAL_BUFFER_SIZE = BUFFER_SIZE + 64;
        const int TARGET_WROTE_BITS = 354848;

    private:
        const std::bitset<64> markers = std::bitset<64>("0100101110111011101110011001100110010101010101010111111111111111");

    private:
        long long counter;

        int64_t bits_wrote;
        std::bitset<64> shifter, pn_shifter;

        int64_t pn_right_bit_counter;
        std::vector<unsigned char> buffer;

        uint64_t now_writing_timestamp;

        uint8_t last_read_bit, last_write_bit;

    public:
        PNSync(int bufsize) : BUFFER_SIZE(bufsize)
        {
            counter = 0;

            buffer.resize(LOCAL_BUFFER_SIZE);
            bits_wrote = -1;

            pn_right_bit_counter = 0;
        }

        ~PNSync() {}

        int process(const uint8_t *data_arrived, size_t data_length, uint8_t **output_pointer)
        {
            const int8_t *signed_buffer = (const int8_t *)data_arrived;

            //    PluginData *pdata = (PluginData *)(acquire_result->plugin_custom_data);
            counter += data_length;
            *output_pointer = buffer.data();

            long long output_buffer_bit_index = 0;

#define PUSHBIT(x, val)                                                                                                                                                                                \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        uint8_t __bit_to_write = x ^ last_write_bit;                                                                                                                                                   \
        last_write_bit = !!__bit_to_write;                                                                                                                                                             \
        buffer[output_buffer_bit_index] = __bit_to_write ? val : -val;                                                                                                                                 \
        output_buffer_bit_index++;                                                                                                                                                                     \
    } while (0)

            for (int i = 0; i < data_length; i++)
            {
                uint8_t now_bit = signed_buffer[i] > 0;
                uint8_t bit = last_read_bit ^ now_bit;
                last_read_bit = now_bit;
                int8_t value = abs(signed_buffer[i]);

                if (value == 0)
                    value = 1;

                uint8_t pn_bit = pn_shifter[13] ^ pn_shifter[14];

                shifter <<= 1;
                shifter |= bit;

                pn_shifter <<= 1;
                pn_shifter |= pn_bit;

                if (bits_wrote >= 0)
                {
                    uint8_t byte_mask = (bits_wrote / 8) & 1;
                    if (20 * 8 <= bits_wrote && bits_wrote < (20 + 8) * 8)
                    {
                        int byte_ind = (bits_wrote - 20 * 8) / 8;
                        int bit_ind = (bits_wrote - 20 * 8) % 8;
                        uint8_t bit_to_write = (((char *)&(now_writing_timestamp))[byte_ind] >> (7 - bit_ind)) & 1;

                        PUSHBIT(bit_to_write ^ pn_bit ^ byte_mask, value);
                    }
                    else
                    {
                        PUSHBIT(bit, value);
                    }
                    bits_wrote++;

                    if (bits_wrote == TARGET_WROTE_BITS)
                    {
                        bits_wrote = -1;
                        pn_right_bit_counter = 0;
                    }

                    continue;
                }
                else
                {
                    if (bit == pn_bit)
                    {
                        if (pn_right_bit_counter < 5000)
                            pn_right_bit_counter++;

                        // if (pn_right_bit_counter == 1000)
                        // {
                        //     acquire_info.print_log(acquire_info.plugin_interface_data, INFO,
                        //                                     "Locked %lld", cross_module_shared_memory[0]);
                        // }
                    }
                    else
                    {
                        pn_right_bit_counter -= 2;
                        if (pn_right_bit_counter <= 0)
                        {
                            pn_right_bit_counter = 1;
                            pn_shifter = shifter;
                        }
                    }

                    PUSHBIT(0, value);

                    if ((markers ^ pn_shifter).count() == 0)
                    {
                        if (pn_right_bit_counter > 1000)
                        {
                            bits_wrote = 0;

                            for (int k = 63; k >= 0; k--)
                                PUSHBIT(markers[k], value);

                            now_writing_timestamp = counter; // cross_module_shared_memory[0];
                        }
                    }
                }
            }

            return output_buffer_bit_index;
        }
    };
} // namespace fengyun_svissr