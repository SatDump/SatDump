#include "ts_demux.h"

namespace mpeg_ts
{
    std::vector<std::vector<uint8_t>> TSDemux::demux(uint8_t *ts, int pid)
    {
        std::vector<std::vector<uint8_t>> frames_out;

        ts_header.parse(ts);

        if (pid != -1 && ts_header.pid != pid)
            return frames_out;

        int offset = 0;

        if (ts_header.afc == 0b10) // Adapation field only, no payload
            return frames_out;
        else if (ts_header.afc == 0b11) // Adapation field followed
            offset = ts[4] + 1;

        if (ts_header.pusi)
        {
            if (offset > 188 - 4)
                return frames_out;

            uint8_t ptr = ts[4 + offset];

            if (offset + ptr > 188 - 5)
                return frames_out;

            if (in_progress)
            {
                current_payload.insert(current_payload.end(), &ts[5 + offset], &ts[5 + offset + ptr]);
                frames_out.push_back(current_payload);
                in_progress = false;
            }

            current_payload.clear();
            current_payload.insert(current_payload.end(), &ts[5 + offset + ptr], &ts[188]);
            in_progress = true;
        }
        else if (in_progress)
        {
            if (offset > 188 - 4)
                return frames_out;

            current_payload.insert(current_payload.end(), &ts[4 + offset], &ts[188]);
        }

        return frames_out;
    }
};