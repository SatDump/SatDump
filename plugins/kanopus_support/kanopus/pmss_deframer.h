#pragma once

#include "utils/binary.h"
#include <cstdint>
#include <cstring>
#include <vector>

namespace kanopus
{
    class PMSSDeframer
    {
    private:
        const uint8_t sync_word[16] = {0xf5, 0x0a, 0xf5, 0x0a, 0xf5, 0x0a, 0xf5, 0x0a, 0xf5, 0x0a, 0xf5, 0x0a, 0xf5, 0x0a, 0xf5, 0x0a};
        uint8_t sync_shifter[16];
        int in_frame = 0;
        int skip = 0;
        std::vector<uint8_t> frm;

        bool synced = false;

    public:
        std::vector<std::vector<uint8_t>> work(uint8_t *buf, int size)
        {
            std::vector<std::vector<uint8_t>> fs;

            for (int b = 0; b < size; b++)
            {
                memmove(sync_shifter, sync_shifter + 1, 15);
                sync_shifter[15] = buf[b];

                if (in_frame)
                {
                    //  output_data.write((char *)&buf[b], 1);
                    frm.push_back(buf[b]);
                    in_frame--;
                    if (in_frame == 0)
                        fs.push_back(frm);
                    continue;
                }

                if (skip > 0)
                {
                    skip--;
                    continue;
                }

                int errors = 0;
                for (int i = 0; i < 16; i++)
                    errors += satdump::count_ones(sync_shifter[i] ^ sync_word[i]);

                if (synced ? (errors < 8) : (errors == 0))
                {
                    in_frame = 1960 - 16;
                    skip = 15;
                    frm.clear();
                    for (int i = 0; i < 16; i++)
                        frm.push_back(sync_shifter[i]);
                    //   output_data.write((char *)sync_shifter, 16);
                    //  logger->info("Got Frame!");

                    if (errors == 0)
                        synced = true;
                }
                else
                {
                    synced = false;
                }
            }

            return fs;
        }
    };
} // namespace kanopus