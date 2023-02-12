#pragma once

#include <vector>
#include <deque>
#include <cstdint>

/*
DVB-S Interleaver / Deinterleaver.
Adapted from GNU Radio.
*/
namespace dvbs
{
    class DVBSInterleaving
    {
    private:
        static const int d_SYNC = 0x47;
        static const int d_NSYNC = 0xB8;
        static const int d_MUX_PKT = 8;

        const int d_blocks = 136;
        const int d_I = 12;
        const int d_M = 17;

        std::vector<std::deque<unsigned char>> d_shift_interleave;
        std::vector<std::deque<unsigned char>> d_shift_deinterleave;

    public:
        DVBSInterleaving()
        {
            // Interleaving

            // Positions are shift registers (FIFOs)
            // of length i*M
            d_shift_interleave.reserve(d_I);
            for (int i = 0; i < d_I; i++)
                d_shift_interleave.emplace_back(d_M * i, 0);

            // Deinterleaving

            // The positions are shift registers (FIFOs)
            // of length i*M
            d_shift_deinterleave.reserve(d_I);
            for (int i = (d_I - 1); i >= 0; i--)
                d_shift_deinterleave.emplace_back(d_M * i, 0);
        }

        void interleave(uint8_t *in, uint8_t *out)
        {
            for (int i = 0; i < 1; i++)
            {
                // Process one block of I symbols
                for (unsigned int j = 0; j < d_shift_interleave.size(); j++)
                {
                    d_shift_interleave[j].push_front(in[(d_I * i) + j]);
                    out[(d_I * i) + j] = d_shift_interleave[j].back();
                    d_shift_interleave[j].pop_back();
                }
            }
        }

        void deinterleave(uint8_t *in, uint8_t *out)
        {
            int count = 0;
            for (int mux_pkt = 0; mux_pkt < d_MUX_PKT; mux_pkt++)
            {
                // This is actually the deinterleaver
                for (int k = 0; k < (d_M * d_I); k++)
                {
                    d_shift_deinterleave[k % d_I].push_back(in[count]);
                    out[count++] = d_shift_deinterleave[k % d_I].front();
                    d_shift_deinterleave[k % d_I].pop_front();
                }
            }
        }
    };
};