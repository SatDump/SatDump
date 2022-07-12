/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include <fstream>
#include <vector>
#include "common/mpeg_ts/ts_header.h"
#include "common/mpeg_ts/ts_demux.h"
#include <map>
#include "libs/bzlib/bzlib.h"
#include "common/image/image.h"
#include <filesystem>

int main(int argc, char *argv[])
{
    initLogger();

    uint8_t mpeg_ts[188];
    std::ifstream ts_file(argv[1]);
    std::ofstream cadu_file("stuff.frm");

    // TS Stuff
    mpeg_ts::TSHeader ts_header;
    mpeg_ts::TSDemux ts_demux;

    while (!ts_file.eof())
    {
        ts_file.read((char *)mpeg_ts, 188);

        ts_header.parse(mpeg_ts);
        // logger->critical(ts_header.pid);

        std::vector<std::vector<uint8_t>> frames = ts_demux.demux(mpeg_ts, 500);

        // logger->critical(frames.size());

        for (std::vector<uint8_t> payload : frames)
        {
            payload.erase(payload.begin(), payload.begin() + 40); // Extract the Fazzt frame

            uint32_t h = payload[0] << 24 | payload[1] << 16 | payload[2] << 8 | payload[3];
            logger->critical(h);
            // logger->critical(payload.size());
            payload.resize(1500);
            if (h == 16837331)
                cadu_file.write((char *)payload.data(), 1500);
        }
    }
}