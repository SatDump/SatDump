#include "ts_demux.h"

namespace mpeg_ts
{
    std::vector<std::vector<uint8_t>> TSDemux::demux(uint8_t *ts, int pid)
    {
        std::vector<std::vector<uint8_t>> frames_out;

        ts_header.parse(ts);

        if (pid != -1 && ts_header.pid != pid)
            return frames_out;

        if (ts_header.pusi)
        {
            uint8_t ptr = ts[4];

            if (ptr > 188 - 5)
                return frames_out;

            if (in_progress)
            {
                current_payload.insert(current_payload.end(), &ts[5], &ts[5 + ptr]);
                frames_out.push_back(current_payload);
                in_progress = false;
            }

            current_payload.clear();
            current_payload.insert(current_payload.end(), &ts[5 + ptr], &ts[188]);
            in_progress = true;
        }
        else if (in_progress)
            current_payload.insert(current_payload.end(), &ts[4], &ts[188]);

        return frames_out;
    }
};