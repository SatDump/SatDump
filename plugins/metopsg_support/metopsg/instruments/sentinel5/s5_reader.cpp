#include "s5_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "common/tracking/interpolator.h"
#include "image/image.h"
#include "image/io.h"
#include "image/processing.h"
#include <cstdio>
#include <fstream>
#include <string>

#include "logger.h"
#include "nlohmann/json_utils.h"
#include "utils/stats.h"

// std::ofstream s5("/tmp/s5.bin");

namespace metopsg
{
    namespace sentinel5
    {
        Sentinel5Reader::Sentinel5Reader() {}

        Sentinel5Reader::~Sentinel5Reader() {}

        void Sentinel5Reader::work(ccsds::CCSDSPacket pkt)
        {
            if (pkt.header.apid == 2047)
                return;

            /////////////////////////////////////////////// TODOREWORK!!!!
            if (pkt.header.apid == 1328)
                return;

            if (pkt.header.sequence_flag == 3)
            {
                work2({pkt});
            }
            else if (pkt.header.sequence_flag == 1)
            {
                reassembled_pkts[pkt.header.apid].clear();
                reassembled_pkts[pkt.header.apid].push_back(pkt);
            }
            else if (pkt.header.sequence_flag == 0 || pkt.header.sequence_flag == 2)
            {
                reassembled_pkts[pkt.header.apid].push_back(pkt);
                if (pkt.header.sequence_flag == 2)
                    work2(reassembled_pkts[pkt.header.apid]);
            }
        }

        void Sentinel5Reader::work2(std::vector<ccsds::CCSDSPacket> pkts)
        {
            if (pkts.size() == 0)
                return;

            auto &pkt = pkts[0];

            if (pkt.payload.size() <= 30)
                return;

            int mkr = pkt.payload[15];
            int mkr2 = pkt.payload[29];

            // for (int i = 0; i < pkts.size(); i++)
            //     all_pkts[std::to_string(mkr)][std::to_string(mkr2)][i].push_back(pkts[i].payload.size());

#if 0
            if (mkr == 206) // && mkr2 == 4)
            {
                pkt.payload.resize(100000 - 6);
                s5.write((char *)pkt.header.raw, 6);
                s5.write((char *)pkt.payload.data(), 100000 - 6);
            }
#endif

            for (auto &ch : all_channels)
            {
                if (mkr != ch.mkr || mkr2 != ch.mkr2)
                    continue;

                if (pkts.size() < ch.pktsize.size())
                {
                    logger->trace("Invalid packet numbers");
                    return;
                }

                for (int i = 0; i < ch.pktsize.size(); i++)
                {
                    pkts[i].payload.resize(ch.pktsize[i]);
                    parseLine(ch, pkts[i], ch.bits, ch.offset);
                }

                ch.lines++;

                return;
            }

            logger->warn("Unknown Sentinel-5 packet! %5d %3d %2d %5d", pkt.header.apid, mkr, mkr2, pkt.payload.size());

            total_packets++;
        }

        image::Image Sentinel5Reader::getChannel(int idx, std::string &name)
        {
            channel_t ch;

            if (idx >= all_channels.size())
                return image::Image();

            ch = all_channels[idx];

            name = std::to_string(ch.mkr) + "_" + std::to_string(ch.mkr2);

#if 0
            nlohmann::json all_pkts_v;
            for (auto &v : all_pkts.items())
            {
                for (auto &vv : v.value().items())
                {
                    for (auto &vvv : vv.value().items())
                    {
                        std::vector<int> vec = vvv.value();
                        all_pkts_v[v.key()][vv.key()][vvv.key()] = satdump::most_common(vec.begin(), vec.end(), vec[0]);
                    }
                }
            }

            saveJsonFile("/tmp/sg3/s5dump.json", all_pkts_v);
#endif

            if (ch.lines == 0)
                return image::Image();

            auto img = image::Image(ch.img.data(), 16, ch.img.size() / ch.lines, ch.lines, 1);
            image::equalize(img);
            return img;
        }
    } // namespace sentinel5
} // namespace metopsg