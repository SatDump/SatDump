#include "hdlc_def.h"

namespace ax25
{
    HDLCDeframer::HDLCDeframer(int length_min, int length_max)
        : d_len_min(length_min), d_len_max(length_max)
    {
        wip_pkt = new uint8_t[d_len_max];
    }

    HDLCDeframer::~HDLCDeframer()
    {
        delete[] wip_pkt;
    }

    std::vector<std::vector<uint8_t>> HDLCDeframer::work(uint8_t *buffer, int size)
    {
        std::vector<std::vector<uint8_t>> frm_out;

        for (int i = 0; i < size; i++)
        {
            uint8_t bit = buffer[i];

            if (consecutive_ones < 5) // Other data
            {
                if (pos_in_pkt > d_len_max)
                {
                    pos_in_pkt = 0;
                    bitpos = 0;
                }

                wip_pkt[pos_in_pkt] = bit << 7 | wip_pkt[pos_in_pkt] >> 1;
                bitpos++;

                if (bitpos == 8)
                {
                    pos_in_pkt++;
                    bitpos = 0;
                }
            }
            else // Frame sync?
            {
                if (bit)
                {
                    if (pos_in_pkt >= d_len_min)
                    {
                        // CRC Check
                        uint16_t crc = wip_pkt[pos_in_pkt - 1] << 8 | wip_pkt[pos_in_pkt - 2];
                        if (crc == crc_ccitt.compute(wip_pkt, pos_in_pkt - 2))
                            frm_out.push_back(std::vector<uint8_t>(wip_pkt, wip_pkt + pos_in_pkt - 2));
                    }

                    pos_in_pkt = 0;
                    bitpos = 0;
                }

                // If not a 1, then it's stuffing. Do nothing
            }

            if (bit)
                consecutive_ones++;
            else
                consecutive_ones = 0;
        }

        return frm_out;
    }
}