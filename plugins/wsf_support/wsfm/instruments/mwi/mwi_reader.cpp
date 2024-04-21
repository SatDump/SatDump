#include "mwi_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

namespace wsfm
{
    namespace mwi
    {
        MWIReader::MWIReader()
        {
            lines = 0;
            timestamps.resize(2, -1);
        }

        MWIReader::~MWIReader()
        {
            for (int i = 0; i < 17; i++)
                channels[i].clear();
        }

        void MWIReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.header.sequence_flag == 0b01)
            {
                if (wip_full_pkt.size() > 0)
                {
                    wip_full_pkt.resize(24798);
                    // data_ou.write((char *)wip_full_pkt.data(), wip_full_pkt.size());

                    repackBytesTo16bits(&wip_full_pkt[524], 24266, (uint16_t *)bufline);
                    for (int i = 0; i < 17; i++)
                        channels[i].insert(channels[i].end(), &bufline[714 * i], &bufline[714 * i + 571]);
                    lines++;

                    double timestamp = ccsds::parseCCSDSTimeFullRaw(wip_full_pkt.data(), -4383);
                    timestamps.push_back(timestamp);
                }
                wip_full_pkt.clear();

                wip_full_pkt.insert(wip_full_pkt.end(), pkt.payload.begin(), pkt.payload.end());
            }
            else if (pkt.header.sequence_flag == 0b00)
            {
                wip_full_pkt.insert(wip_full_pkt.end(), pkt.payload.begin(), pkt.payload.end());
            }
            else if (pkt.header.sequence_flag == 0b10)
            {
                wip_full_pkt.insert(wip_full_pkt.end(), pkt.payload.begin(), pkt.payload.end());
                if (wip_full_pkt.size() > 0)
                {
                    wip_full_pkt.resize(24798);
                    // data_ou.write((char *)wip_full_pkt.data(), wip_full_pkt.size());

                    repackBytesTo16bits(&wip_full_pkt[524], 24266, (uint16_t *)bufline);
                    for (int i = 0; i < 17; i++)
                        channels[i].insert(channels[i].end(), &bufline[714 * i], &bufline[714 * i + 571]);
                    lines++;

                    double timestamp = ccsds::parseCCSDSTimeFullRaw(wip_full_pkt.data(), -4383);
                    timestamps.push_back(timestamp);
                }
                wip_full_pkt.clear();
            }
        }

        image::Image<uint16_t> MWIReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].data(), 571, lines, 1);
        }
    }
}