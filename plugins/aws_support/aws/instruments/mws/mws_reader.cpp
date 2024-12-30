#include "mws_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

#include <cstdio>

namespace aws
{
    namespace mws
    {
        MWSReader::MWSReader()
        {
            lines = 0;
            timestamps.resize(2, -1);
        }

        MWSReader::~MWSReader()
        {
            for (int i = 0; i < 19; i++)
                channels[i].clear();
        }

        double parseCUC(uint8_t *dat)
        {
            double seconds = dat[1] << 24 |
                             dat[2] << 16 |
                             dat[3] << 8 |
                             dat[4];
            double fseconds = dat[5] << 16 |
                              dat[6] << 8 |
                              dat[7];
            double timestamp = seconds + fseconds / 16777215.0 + 3657 * 24 * 3600;
            if (timestamp > 0)
                return timestamp;
            else
                return -1;
        }

        void MWSReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.payload.size() < 19)
                return;

            if (pkt.header.sequence_flag == 0b01)
            {
                if (wip_full_pkt.size() > 0)
                {
                    wip_full_pkt.resize(7599);

                    for (int c = 0; c < 145; c++)
                        for (int i = 0; i < 19; i++)
                            channels[i].push_back((wip_full_pkt[1799 + (c * 40) + (i * 2) + 1] << 8) | wip_full_pkt[1799 + (c * 40) + (i * 2) + 0]);
                    lines++;

                    double timestamp = parseCUC(wip_full_pkt.data() + 191);
                    if (crc.check(pkt))
                        timestamps.push_back(timestamp);
                    else
                        timestamps.push_back(-1);
                }
                wip_full_pkt.clear();

                wip_full_pkt.insert(wip_full_pkt.end(), pkt.payload.begin() + 17, pkt.payload.end() - 2);
            }
            else if (pkt.header.sequence_flag == 0b00)
            {
                wip_full_pkt.insert(wip_full_pkt.end(), pkt.payload.begin() + 17, pkt.payload.end() - 2);
            }
            else if (pkt.header.sequence_flag == 0b10)
            {
                wip_full_pkt.insert(wip_full_pkt.end(), pkt.payload.begin() + 17, pkt.payload.end() - 2);
                if (wip_full_pkt.size() > 0)
                {
                    wip_full_pkt.resize(7599);

                    for (int c = 0; c < 145; c++)
                        for (int i = 0; i < 19; i++)
                            channels[i].push_back((wip_full_pkt[1799 + (c * 40) + (i * 2) + 1] << 8) | wip_full_pkt[1799 + (c * 40) + (i * 2) + 0]);
                    lines++;

                    double timestamp = parseCUC(wip_full_pkt.data() + 191);
                    if (crc.check(pkt))
                        timestamps.push_back(timestamp);
                    else
                        timestamps.push_back(-1);

                    //                    printf("%d - ", int(wip_full_pkt[44] << 8 | wip_full_pkt[45]));
                    //                    printf("%d - ", int(wip_full_pkt[46] << 8 | wip_full_pkt[47]));
                    //                    printf("%d - ", int(wip_full_pkt[48] << 8 | wip_full_pkt[49]));
                    //                    printf("%d\n", int(wip_full_pkt[50] << 8 | wip_full_pkt[51]));
                }
                wip_full_pkt.clear();
            }
        }

        image::Image MWSReader::getChannel(int channel)
        {
            auto img = image::Image(channels[channel].data(), 16, 145, lines, 1);
            img.mirror(true, false);
            return img;
        }
    }
}