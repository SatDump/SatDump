#pragma once

#include <cstdint>

namespace fengyun_svissr
{
    class PNDerandomizer
    {
    private:
        uint8_t *derandTable;

    public:
        PNDerandomizer();
        ~PNDerandomizer();
        void derandData(uint8_t *frame, int length);
    };
}