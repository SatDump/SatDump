#include "metimage_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "image/image.h"

namespace metopsg
{
    namespace metimage
    {
        METimageReader::METimageReader()
        {
            for (int c = 0; c < 20; c++)
                for (int l = 0; l < 24; l++)
                    wip_lines[c][l].resize(LWIDTH);
            segments = 0;
        }

        METimageReader::~METimageReader() {}

        void METimageReader::work(ccsds::CCSDSPacket &pkt)
        {
            if ((pkt.header.apid == 1443 || pkt.header.apid == 1444) && pkt.payload.size() >= 20)
            {
                int ch = pkt.payload[15];                            // Channel marker
                int offset = pkt.payload[19] << 8 | pkt.payload[20]; // Current segment offset in pixels

                // On a new scan start, push what we got
                if (offset == 0 && ch == 30)
                {
                    for (int c = 0; c < 20; c++)
                    {
                        for (int l = 0; l < 24; l++)
                        {
                            std::reverse(wip_lines[c][l].begin(), wip_lines[c][l].end());
                            channels[c].insert(channels[c].end(), wip_lines[c][l].begin() + 56, wip_lines[c][l].end());
                            for (int i = 0; i < LWIDTH; i++)
                                wip_lines[c][l][i] = 0;
                        }
                    }

                    segments++;
                }

                // Get channel config
                if (!ch_cfg.count(ch))
                    return;
                auto &cfg = ch_cfg[ch];

                // Safety check offset
                if (offset + SUBPKTN > LWIDTH)
                    return;

                // Resize for safety
                if (cfg.depth == 17)
                    pkt.payload.resize(3027 - 6);
                else if (cfg.depth == 18)
                    pkt.payload.resize(3202 - 6);

                if (cfg.depth == 17)
                {
                    uint32_t vals[1400];
                    repackBytesTo17bits(&pkt.payload[44], 2975, vals);
                    for (int i = 0; i < SUBPKTN; i++)
                        for (int l = 0; l < 24; l++)
                            wip_lines[cfg.index][l][offset + i] = vals[i * 25 + 1 + (cfg.rev ? (23 - l) : l)] >> 1;
                }
                else if (cfg.depth == 18)
                {
                    uint32_t vals[1400];
                    repackBytesTo18bits(&pkt.payload[44], 3150, vals);
                    for (int i = 0; i < SUBPKTN; i++)
                        for (int l = 0; l < 24; l++)
                            wip_lines[cfg.index][l][offset + i] = vals[i * 25 + 1 + (cfg.rev ? (23 - l) : l)] >> 2;
                }
            }
        }

        image::Image METimageReader::getChannel(int c)
        {
            image::Image img(channels[c].data(), 16, LWIDTH - 56 /*TODOREWORK clean this up*/, segments * 24, 1);
            // img.crop(0, 0, 95, img.height());
            return img;
        }
    } // namespace metimage
} // namespace metopsg