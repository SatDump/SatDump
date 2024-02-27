#include "lrpt_msumr_reader.h"
#include "logger.h"
#include <ctime>
#include <cmath>

#define SEG_CNT 20000 // 12250

namespace meteor
{
    namespace msumr
    {
        namespace lrpt
        {
            MSUMRReader::MSUMRReader(bool meteorm2x_mode) : meteorm2x_mode(meteorm2x_mode)
            {
                for (int i = 0; i < 6; i++)
                {
                    channels[i] = new unsigned char[((SEG_CNT / 14) * 8) * 1568];
                    lines[i] = 0;
                    segments[i] = new Segment[SEG_CNT];
                    firstSeg[i] = 4294967295;
                    lastSeg[i] = 0;
                    segCount[i] = 0;
                    rollover[i] = 0;
                    offset[i] = 0;
                }

                // Fetch current day
                time_t currentDay = time(0) + 3 * 3600.0;
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
                // if (packet.payload.size() - 1 != packet.header.packet_length)
                //     return;

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

                Segment newSeg(&packet.payload.data()[0], packet.payload.size(),
                    packet.payload.size() - 1 != packet.header.packet_length, meteorm2x_mode);

                if (!newSeg.isValid())
                    return;

                uint16_t sequence = packet.header.packet_sequence_count;
                uint32_t mcuNumber = newSeg.MCUN;

                if (lastSeq[currentChannel] > sequence && lastSeq[currentChannel] > 10000 && sequence < 1000)
                {
                    rollover[currentChannel] += 16384;
                }

                if (mcuNumber == 0 && offset[currentChannel] == 0)
                {
                    offset[currentChannel] = (sequence + rollover[currentChannel]) % 43 % 14;
                }

                uint32_t id = ((sequence + rollover[currentChannel] - offset[currentChannel]) / 43 * 14) + mcuNumber / 14;

                if (id >= SEG_CNT)
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

            image::Image<uint8_t> MSUMRReader::getChannel(int channel, int max_correct, int32_t first, int32_t last, int32_t offsett)
            {
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

                std::set<uint32_t> bad_px;
                uint32_t index = 0;
                if(lastSeg_l != 0)
                    timestamps.clear();
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

                                if(segments[channel][x + j].partial)
                                    for (uint32_t f = 0; f < 14 * 8; f++)
                                        bad_px.insert(index + f);

                                if (!hasDoneTimestamps)
                                {
                                    if (meteorm2x_mode)
                                        timestamps.push_back(segments[channel][x + j].timestamp);
                                    else
                                        timestamps.push_back(dayValue + segments[channel][x + j].timestamp - 3 * 3600);
                                    hasDoneTimestamps = true;
                                }
                            }
                            else
                            {
                                for (uint32_t f = 0; f < 14 * 8; f++)
                                {
                                    channels[channel][index + f] = 0;
                                    bad_px.insert(index + f);
                                }
                            }

                            index += 14 * 8;
                        }
                    }

                    if (!hasDoneTimestamps)
                    {
                        timestamps.push_back(-1);
                    }
                }

                image::Image<uint8_t> ret = image::Image<uint8_t>(channels[channel], 1568, lines[channel], 1);
                if (max_correct > 0 && ret.size() > 0)
                {
                    logger->info("Filling missing data in channel %d...", channel + 1);

                    // Interpolate image
                    size_t width = ret.width();
                    size_t height = ret.height();
#pragma omp parallel for
                    for (int x = 0; x < width; x++)
                    {
                        int last_good = -1;
                        bool found_bad = false;
                        for (size_t y = 0; y < height; y++)
                        {
                            if (found_bad)
                            {
                                if (bad_px.find(y * width + x) == bad_px.end())
                                {
                                    size_t range = y - last_good;
                                    if (last_good != -1 && range <= max_correct)
                                    {
                                        uint8_t top_val = ret[last_good * width + x];
                                        uint8_t bottom_val = ret[y * width + x];
                                        for (size_t fix_y = 1; fix_y < range; fix_y++)
                                        {
                                            float percent = (float)fix_y / (float)range;
                                            ret[(fix_y + last_good) * width + x] = (1.0f - percent) * (float)top_val + percent * (float)bottom_val;
                                        }
                                    }
                                    found_bad = false;
                                    last_good = y;
                                }
                            }
                            else if (bad_px.find(y * width + x) != bad_px.end())
                                found_bad = true;
                            else
                                last_good = y;
                        }
                    }

                    // Interpolate timestamps
                    int last_good = -1;
                    bool found_bad = false;
                    for (size_t i = 0; i < timestamps.size(); i++)
                    {
                        if (found_bad)
                        {
                            if (timestamps[i] != -1)
                            {
                                if (last_good != -1)
                                    for (size_t j = 1; j < i - last_good; j++)
                                        timestamps[last_good + j] = timestamps[last_good] + (timestamps[i] - timestamps[last_good]) * ((double)j / double(i - last_good));

                                last_good = i;
                                found_bad = false;
                            }
                        }
                        else if (timestamps[i] == -1)
                            found_bad = true;
                        else
                            last_good = i;
                    }

                    // Round all timestamps to 3 decimal places to ensure made-up timestamps
                    // Match with timestamps in other channels
                    for (double &timestamp : timestamps)
                        timestamp = std::floor(timestamp * 1000) / 1000;
                }

                return ret;
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