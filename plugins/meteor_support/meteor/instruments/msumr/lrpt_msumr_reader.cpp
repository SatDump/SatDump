#include "lrpt_msumr_reader.h"
#include <ctime>

namespace meteor
{
    namespace msumr
    {
        namespace lrpt
        {
            MSUMRReader::MSUMRReader()
            {
                for (int i = 0; i < 6; i++)
                {
                    channels[i] = new unsigned char[20000 * 1568];
                    lines[i] = 0;
                    segments[i] = new Segment[200000];
                    firstSeg[i] = 4294967295;
                    lastSeg[i] = 0;
                    segCount[i] = 0;
                    rollover[i] = 0;
                    offset[i] = 0;
                }

                // Fetch current day
                time_t currentDay = time(0);
                dayValue = currentDay - (currentDay % 86400); // Requires the day to be known from another source
            }

            MSUMRReader::~MSUMRReader()
            {
                for (int i = 0; i < 6; i++)
                {
                    delete[] channels[i];
                    delete[] segments[i];
                }
            }

            void MSUMRReader::work(ccsds::CCSDSPacket &packet)
            {
                //if (packet.payload.size() - 1 != packet.header.packet_length)
                //    return;

                int currentChannel = -1;

                if (packet.header.apid == 64)
                    currentChannel = 0;
                else if (packet.header.apid == 65)
                    currentChannel = 1;
                else if (packet.header.apid == 66)
                    currentChannel = 2;
                else if (packet.header.apid == 67)
                    currentChannel = 3;
                else if (packet.header.apid == 68)
                    currentChannel = 4;
                else if (packet.header.apid == 69)
                    currentChannel = 5;
                else
                    return;

                Segment newSeg(&packet.payload.data()[0], packet.payload.size());

                if (!newSeg.isValid())
                    return;

                uint16_t sequence = packet.header.packet_sequence_count;
                uint32_t mcuNumber = newSeg.MCUN;

                if (lastSeq[currentChannel] > sequence && lastSeq[currentChannel] > 16000 && sequence < 1000)
                {
                    rollover[currentChannel] += 16384;
                }

                if (mcuNumber == 0 && offset[currentChannel] == 0)
                {
                    offset[currentChannel] = (sequence + rollover[currentChannel]) % 43 % 14;
                }

                uint32_t id = ((sequence + rollover[currentChannel] - offset[currentChannel]) / 43 * 14) + mcuNumber / 14;

                if (id >= 200000)
                    return;

                if (lastSeg[currentChannel] < id)
                {
                    lastSeg[currentChannel] = id;
                }

                if (firstSeg[currentChannel] > id)
                {
                    firstSeg[currentChannel] = id;
                }

                lastSeq[currentChannel] = sequence;
                segments[currentChannel][id] = newSeg;
                segCount[currentChannel]++;
            }

            image::Image<uint8_t> MSUMRReader::getChannel(int channel, int32_t first, int32_t last, int32_t offsett)
            {
                timestamps.clear();

                uint32_t firstSeg_l;
                uint32_t lastSeg_l;

                if (first == -1 || last == -1 || offsett == -1)
                {
                    firstSeg_l = firstSeg[channel];
                    lastSeg_l = lastSeg[channel];
                }
                else
                {
                    firstSeg_l = ((first + (offsett - (offset[channel]))) * 14);
                    lastSeg_l = ((last + (offsett - (offset[channel]))) * 14);
                }

                firstSeg_l -= firstSeg_l % 14;
                lastSeg_l -= lastSeg_l % 14;

                lines[channel] = ((lastSeg_l - firstSeg_l) / 14) * 8;

                uint32_t index = 0;
                for (uint32_t x = firstSeg_l; x < lastSeg_l; x += 14)
                {
                    bool hasDoneTimestamps = false;
                    for (uint32_t i = 0; i < 8; i++)
                    {
                        for (uint32_t j = 0; j < 14; j++)
                        {
                            if (segments[channel][x + j].isValid())
                            {
                                for (int f = 0; f < 14 * 8; f++)
                                    channels[channel][index + f] = segments[channel][x + j].lines[i][f];

                                if (!hasDoneTimestamps)
                                {
                                    for (int i = -4; i < 4; i++)
                                        timestamps.push_back(dayValue + segments[channel][x + j].timestamp + i * 0.153 - 3 * 3600);
                                    hasDoneTimestamps = true;
                                }
                            }
                            else
                            {
                                for (int f = 0; f < 14 * 8; f++)
                                    channels[channel][index + f] = 0;
                            }

                            index += 14 * 8;
                        }
                    }

                    if (!hasDoneTimestamps)
                    {
                        for (int i = -4; i < 4; i++)
                            timestamps.push_back(-1);
                    }
                }

                return image::Image<uint8_t>(channels[channel], 1568, lines[channel], 1);
            }

            std::array<int32_t, 3> MSUMRReader::correlateChannels(int channel1, int channel2)
            {
                int commonFirstScan = std::max(firstSeg[channel1] / 14, firstSeg[channel2] / 14);
                int commonLastScan = std::min(lastSeg[channel1] / 14, lastSeg[channel2] / 14);
                int commonOffsetScan = std::max(offset[channel1], offset[channel2]);
                return {commonFirstScan, commonLastScan, commonOffsetScan};
            }

            std::array<int32_t, 3> MSUMRReader::correlateChannels(int channel1, int channel2, int channel3)
            {
                int commonFirstScan = std::max(firstSeg[channel1] / 14, std::max(firstSeg[channel2] / 14, firstSeg[channel3] / 14));
                int commonLastScan = std::min(lastSeg[channel1] / 14, std::min(lastSeg[channel2] / 14, lastSeg[channel3] / 14));
                int commonOffsetScan = std::max(offset[channel1], std::max(offset[channel2], offset[channel3]));
                return {commonFirstScan, commonLastScan, commonOffsetScan};
            }
        } // namespace lrpt
    }     // namespace msumr
} // namespace meteor