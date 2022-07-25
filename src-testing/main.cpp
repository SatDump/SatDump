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
#include "common/mpeg_ts/ts_header.h"
#include "common/mpeg_ts/ts_demux.h"
#include "common/mpeg_ts/dvb_mpe.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream eumetcast_ts("/home/alan/Downloads/0.0E_11262.568_H_32999_(2022-07-11 21.30.11)_dump.ts");
    std::ofstream eumetcast_ttttt("eumetcast_frm.bin");

    uint8_t ts_frame[188];
    mpeg_ts::TSHeader tsheader;
    mpeg_ts::TSDemux tsdemux;

    mpeg_ts::MPEHeader mpeheader;
    mpeg_ts::IPv4Header ipv4header;

    while (!eumetcast_ts.eof())
    {
        eumetcast_ts.read((char *)ts_frame, 188);

        std::vector<std::vector<uint8_t>> framesss = tsdemux.demux(ts_frame, 500);
        for (std::vector<uint8_t> &frm : framesss)
        {
            // uint32_t marker = frm[40] << 24 | frm[41] << 16 | frm[42] << 8 | frm[43];

            // logger->info(marker);

            mpeheader.parse(&frm[0]);
            ipv4header.parse(&frm[12]);

            frm.resize(4000 + 40);

            if (ipv4header.target_ip_1 == 224 && ipv4header.target_ip_2 == 223 && ipv4header.target_ip_3 == 222 && ipv4header.target_ip_4 == 27 /*&& frm[42] == 0xa1 && frm[43] == 0x5a*/)
            {
                logger->critical("{:d} {:d} {:d}    TARGET IP {:d}.{:d}.{:d}.{:d}", mpeheader.section_length, frm.size(), ipv4header.ihl, ipv4header.target_ip_1, ipv4header.target_ip_2, ipv4header.target_ip_3, ipv4header.target_ip_4);
                //  logger->critical(frm.size());
                eumetcast_ttttt.write((char *)&frm[40], 4000);
            }
        }
    }
}