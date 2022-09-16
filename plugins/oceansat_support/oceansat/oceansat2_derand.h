#pragma once

#include <cstdint>

namespace oceansat
{
    class Oceansat2Derand
    {
    private:
        static bool pn_sequence[2047];
        uint8_t mask[102350];

    public:
        Oceansat2Derand();
        void work(uint8_t *data);
    };
} // namespace oceansat