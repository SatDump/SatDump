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
#include <array>
#include <algorithm>

#include "common/ccsds/ccsds_weather/vcdu.h"
#include "common/ccsds/ccsds_weather/demuxer.h"

int main(int argc, char *argv[])
{
    initLogger();

    uint8_t cadu[1024];

    std::ifstream data_in(argv[1], std::ios::binary);

    std::ofstream data_out(argv[2], std::ios::binary);

    ccsds::ccsds_weather::Demuxer demuxer_vcid4(884, true, 0);

    std::vector<uint8_t> wip_payload;

    int payload_cnt = 0;

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)cadu, 1024);

        // Parse this transport frame
        ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

        printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 4)
        {
            // data_out.write((char *)cadu, 1024);

            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid4.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                printf("APID %d\n", pkt.header.apid);

                if (pkt.header.apid == 459)
                {

                    printf("LEN %d\n", pkt.payload.size());

                    // if (pkt.payload[4 + 0] == 0xFF && pkt.payload[4 + 1] == 0xD8 && pkt.payload[4 + 2] == 0xFF)
                    if (pkt.payload.size() == 2036)
                    {
                        pkt.payload.resize(3000);
                        data_out.write((char *)pkt.header.raw, 6);
                        data_out.write((char *)pkt.payload.data(), 3000);
                    }

                    if (pkt.header.sequence_flag == 0b01)
                    {
                        if (wip_payload.size() > 0)
                        {
                            int pos = 0;

                            for (int i = 0; i < wip_payload.size() - 3; i++)
                                if (wip_payload[i + 0] == 0xFF && wip_payload[i + 1] == 0xD8 && wip_payload[i + 2] == 0xFF)
                                    pos = i + 1;

                            wip_payload.erase(wip_payload.begin(), wip_payload.begin() + pos);

                            std::ofstream("hinode_out/" + std::to_string(payload_cnt++) + ".jpg", std::ios::binary).write((char *)wip_payload.data(), wip_payload.size());
                        }

                        wip_payload.clear();

                        wip_payload.insert(wip_payload.end(), pkt.payload.begin() + 4, pkt.payload.end());
                    }
                    else if (pkt.header.sequence_flag == 0b00)
                    {
                        wip_payload.insert(wip_payload.end(), pkt.payload.begin() + 4, pkt.payload.end());
                    }
                    else if (pkt.header.sequence_flag == 0b10)
                    {
                        wip_payload.insert(wip_payload.end(), pkt.payload.begin() + 4, pkt.payload.end());

                        if (wip_payload.size() > 0)
                        {
                            int pos = 0;

                            for (int i = 0; i < wip_payload.size() - 3; i++)
                                if (wip_payload[i + 0] == 0xFF && wip_payload[i + 1] == 0xD8 && wip_payload[i + 2] == 0xFF)
                                    pos = i;

                            wip_payload.erase(wip_payload.begin(), wip_payload.begin() + pos);

                            std::ofstream("hinode_out/" + std::to_string(payload_cnt++) + ".jpg", std::ios::binary).write((char *)wip_payload.data(), wip_payload.size());

                            wip_payload.clear();
                        }
                    }
                }
            }
        }
    }
}
