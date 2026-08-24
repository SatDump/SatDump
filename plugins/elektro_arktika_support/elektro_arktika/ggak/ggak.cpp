#include "ggak.h"
#include <cstdint>

namespace elektro_arktika
{
    namespace ggak
    {
        double getTimestamp(GGAKFrame *frm) { return (double)frm->timestamp + 10957. * 3600. * 24. - 3. * 3600.; }

        bool checkCRC(GGAKFrame *frm)
        {
            uint8_t *cadu = (uint8_t *)frm;
            uint16_t sum = 0;
            for (size_t i = 0; i < 222; i++)
                sum += cadu[i];
            return sum == (cadu[222] << 8 | cadu[223]);
        }
    } // namespace ggak
} // namespace elektro_arktika