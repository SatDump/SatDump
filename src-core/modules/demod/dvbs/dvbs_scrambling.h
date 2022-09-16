#pragma once

#include <cstdint>

/*
DVB-S Descrambler
*/
namespace dvbs
{
    class DVBSScrambling
    {
    private:
        int reg = 0;

        int prbs(int clocks)
        {
            int res = 0;
            int feedback = 0;

            for (int i = 0; i < clocks; i++)
            {
                feedback = ((reg >> (14 - 1)) ^ (reg >> (15 - 1))) & 0x1;
                reg = ((reg << 1) | feedback) & 0x7fff;
                res = (res << 1) | feedback;
            }

            return res;
        }

    public:
        void descramble(uint8_t *frm)
        {
            for (int pkt = 0; pkt < 8; pkt++)
            {
                int outc = 0;

                if (frm[pkt * 204 + outc] == 0xB8)
                    reg = 0xa9; // Reset
                else
                    prbs(8); // Skip

                frm[pkt * 204 + outc++] = 0x47;

                for (int k = 1; k < 188; k++)
                    frm[pkt * 204 + outc++] ^= prbs(8);
            }
        }
    };
};