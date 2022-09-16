#include "ts_header.h"

namespace mpeg_ts
{
    void TSHeader::parse(uint8_t *ts)
    {
        sync = ts[0];
        tei = ts[1] >> 7;
        pusi = (ts[1] >> 6) & 1;
        tp = (ts[1] >> 5) & 1;
        pid = (ts[1] & 0b11111) << 8 | ts[2];
        tsc = (ts[3] >> 6) & 0b11;
        afc = (ts[3] >> 4) & 0b11;
        cont = ts[3] & 0x0F;
    }
};