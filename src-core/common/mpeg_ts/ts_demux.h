#pragma once

#include <cstdint>
#include <vector>
#include "ts_header.h"

namespace mpeg_ts
{
    class TSDemux
    {
    private:
        bool in_progress = false;
        std::vector<uint8_t> current_payload;
        mpeg_ts::TSHeader ts_header;

    public:
        TSDemux() {}
        ~TSDemux() {}

        std::vector<std::vector<uint8_t>> demux(uint8_t *ts, int pid = -1);
    };
};