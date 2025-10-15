#include "mws_reader.h"
#include "image/image.h"
#include "metopsg/instruments/cuc.h"
#include <algorithm>
#include <cmath>

namespace metopsg
{
    namespace mws
    {
        MWSReader::MWSReader()
        {
            for (int c = 0; c < 24; c++)
                lines[c] = 0;
        }

        MWSReader::~MWSReader() {}

        void MWSReader::work(ccsds::CCSDSPacket &pkt)
        {
            double timestamp = parseCUC(&pkt.payload[3]);

            if (pkt.header.apid == 1105 || pkt.header.apid == 1106)
            {
                if (pkt.payload.size() < 3388)
                    return;
                int ch = pkt.header.apid - 1105;
                for (int i = 0; i < 1680; i++)
                    channels[ch].push_back(uint16_t(pkt.payload[28 + i * 2 + 0] << 8 | pkt.payload[28 + i * 2 + 1]) - 32768);
                timestamps[ch].push_back(timestamp);
                lines[ch]++;
            }
            else if (pkt.header.apid == 1107)
            {
                if (pkt.payload.size() < 2968)
                    return;
                for (int c = 0; c < 14; c++)
                {
                    for (int i = 0; i < 105; i++)
                        channels[2 + c].push_back(uint16_t(pkt.payload[28 + c * 105 * 2 + i * 2 + 0] << 8 | pkt.payload[28 + c * 105 * 2 + i * 2 + 1]) - 32768);
                    timestamps[2 + c].push_back(timestamp);
                    lines[2 + c]++;
                }
            }
            else if (pkt.header.apid == 1108)
            {
                if (pkt.payload.size() < 238)
                    return;
                for (int i = 0; i < 105; i++)
                    channels[16].push_back(uint16_t(pkt.payload[28 + i * 2 + 0] << 8 | pkt.payload[28 + i * 2 + 1]) - 32768);
                timestamps[16].push_back(timestamp);
                lines[16]++;
            }
            else if (pkt.header.apid == 1109)
            {
                if (pkt.payload.size() < 238)
                    return;
                for (int i = 0; i < 105; i++)
                    channels[17].push_back(uint16_t(pkt.payload[28 + i * 2 + 0] << 8 | pkt.payload[28 + i * 2 + 1]) - 32768);
                timestamps[17].push_back(timestamp);
                lines[17]++;
            }
            else if (pkt.header.apid == 1110)
            {
                if (pkt.payload.size() < 1078)
                    return;
                for (int c = 0; c < 5; c++)
                {
                    for (int i = 0; i < 105; i++)
                        channels[18 + c].push_back(uint16_t(pkt.payload[28 + c * 105 * 2 + i * 2 + 0] << 8 | pkt.payload[28 + c * 105 * 2 + i * 2 + 1]) - 32768);
                    timestamps[18 + c].push_back(timestamp);
                    lines[18 + c]++;
                }
            }
            else if (pkt.header.apid == 1111)
            {
                if (pkt.payload.size() < 238)
                    return;
                for (int i = 0; i < 105; i++)
                    channels[23].push_back(uint16_t(pkt.payload[28 + i * 2 + 0] << 8 | pkt.payload[28 + i * 2 + 1]) - 32768);
                timestamps[23].push_back(timestamp);
                lines[23]++;
            }
        }

        void MWSReader::correlate()
        {
            std::vector<double> all_timestamps;

            for (int c = 0; c < 24; c++)
            {
                for (auto &f : timestamps[c])
                {
                    bool contains = false;
                    for (auto &t : all_timestamps)
                        if (fabs(t - f) < 0.01)
                            contains = true;
                    if (!contains)
                        all_timestamps.push_back(f);
                }
            }

            std::sort(all_timestamps.begin(), all_timestamps.end());

            for (int c = 0; c < 24; c++)
            {
                std::vector<uint16_t> newchannel;
                int size = c < 2 ? 1680 : 105;

                for (auto &t : all_timestamps)
                {
                    bool had = false;
                    for (int l = 0; l < lines[c]; l++)
                        if (fabs(timestamps[c][l] - t) < 0.01)
                        {
                            newchannel.insert(newchannel.end(), channels[c].begin() + l * size, channels[c].begin() + (l + 1) * size);
                            had = true;
                        }
                    if (!had)
                        newchannel.resize(newchannel.size() + size, 0);
                }

                channels[c] = newchannel;
                timestamps[c] = all_timestamps;
            }
        }

        image::Image MWSReader::getChannel(int c)
        {
            if (c == 0 || c == 1)
            {
                image::Image img(channels[c].data(), 16, 1680, lines[c], 1);
                img.crop(0, 0, 1520, img.height());
                img.mirror(1, 0);
                return img;
            }
            else
            {
                image::Image img(channels[c].data(), 16, 105, lines[c], 1);
                img.crop(0, 0, 95, img.height());
                img.mirror(1, 0);
                return img;
            }
        }
    } // namespace mws
} // namespace metopsg