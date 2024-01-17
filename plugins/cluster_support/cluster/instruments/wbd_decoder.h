#pragma once
#include <vector>
#include <cstdint>

namespace cluster
{
    struct MajorFrame
    {
        std::vector<uint8_t> payload;
        bool VCXO = false;
        bool ADpower = false;
        bool AGC = false;
        bool OBDH = false;
        bool commands = false;
        int convfreq = 0;
        int outputmode = 0;
        int antennaselect = 0;
        int WBDmodel = 0;
        int gainlevel = 0;
        int lwagc = 0;
        int hgagc = 0;
    };

    class WBDdecoder
    {
    private:
        MajorFrame wbdwip;

    public:
        std::vector<MajorFrame> work(uint8_t *frm);
    };
}