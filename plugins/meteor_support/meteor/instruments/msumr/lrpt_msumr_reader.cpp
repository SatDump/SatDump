#include "lrpt_msumr_reader.h"
#include "logger.h"
#include "common/utils.h"
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
                    firstSeg[i] = offset[i] = 4294967295;
                    rollover[i] = lastSeq[i] = lastSeg[i] = lines[i] = 0;
                }

                // Fetch current day, moscow time.
                // Only used for old M2 LRPT
                time_t currentDay = time(0) + 3 * 3600.0;
                dayValue = currentDay - (currentDay % 86400);
            }

            MSUMRReader::~MSUMRReader()
            {
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
                uint32_t mcu_count = newSeg.MCUN / 14;

                // Detect sequence count rollover. 15% threshold at each end
                // to prevent false positives
                if (lastSeq[currentChannel] > sequence && lastSeq[currentChannel] > 13926 && sequence < 2458)
                {
                    rollover[currentChannel] += 16384;
                }

                // Extrapolate the offset, which is what the sequence counter was when this
                // channel first appeared after sequence count 0
                if (offset[currentChannel] == 4294967295)
                {
                    uint32_t mcu_seq = sequence + (mcu_count > sequence ? 16384 : 0) - mcu_count;
                    offset[currentChannel] = (mcu_seq + rollover[currentChannel]) % 43;
                }

                // Transmission loop is 43 packets long (14 segments per line * 3 channels + 1 telemetry packet)
                // Based on that assumption, computing the ID here effectively makes a new sequence unique per
                // channel that is continuous from the end of one line to the beginning of another
                uint32_t id = ((sequence + rollover[currentChannel] - offset[currentChannel]) / 43) * 14 + mcu_count;
                uint32_t new_firstSeg = (firstSeg[currentChannel] > id ? id : firstSeg[currentChannel]);
                uint32_t new_lastSeg = (lastSeg[currentChannel] < id ? id : lastSeg[currentChannel]);

                // Ensures data errors do not try to generate images that are too tall. This limits LRPT image
                // height to 11428 px [(20000/14) * 8], which is significantly taller than LRPT physically can be.
                // Unless you get another satellite to tag along and continuously receive it. In which case, can we
                // be friends?
                if (new_lastSeg - new_firstSeg > SEG_CNT)
                    return;

                firstSeg[currentChannel] = new_firstSeg;
                lastSeg[currentChannel] = new_lastSeg;
                lastSeq[currentChannel] = sequence;
                segments[currentChannel][id] = newSeg;
            }

            image::Image MSUMRReader::getChannel(int channel, size_t max_correct)
            {
                uint32_t firstSeg_l = 4294967295;
                uint32_t lastSeg_l = 0;
                int lowest_offset = 6;
                int lowest_channel = 6;

                // Check all channels for first/last segment and alignment info
                for (int i = 0; i < 6; i++)
                {
                    // Not a valid channel
                    if (offset[i] == 4294967295)
                        continue;

                    // Get the lowest channel transmitted
                    if (lowest_channel == 6)
                        lowest_channel = i;

                    // Find the point where the satellite's channel loop restarts (used for alignment)
                    if (offset[i] < (lowest_offset == 6 ? 43 : offset[lowest_offset]))
                        lowest_offset = i;

                    // Determine first and last segment
                    if (firstSeg[i] < firstSeg_l)
                        firstSeg_l = firstSeg[i];
                    if (lastSeg[i] > lastSeg_l)
                        lastSeg_l = lastSeg[i];
                }

                // Align channel to others based on channel loop order. Only needed if the first channel transmitted
                // does not have the lowest offset. This happens when the first or second channel in the loop was
                // first transmitted in the previous sequence run.
                if (lowest_channel != lowest_offset)
                {
                    if (channel < lowest_offset)
                        firstSeg_l -= 14;
                    else
                        lastSeg_l += 14;
                }

                // One line after the last line due to the loops below
                // (first line is included; last is not, make the upper bound one past the end)
                lastSeg_l += 14;

                // Check if the current channel has any data
                if (firstSeg[channel] == 4294967295)
                    firstSeg_l = 0;
                if (lastSeg[channel] == 0)
                    lastSeg_l = 0;

                // Get first segment of first/last line
                firstSeg_l -= firstSeg_l % 14;
                lastSeg_l -= lastSeg_l % 14;

                // Initialize image info
                // Width  = 8 px per MCU * 14 MCUs wide per segment * 14 segments per line = 1568
                // Height = 8 px per MCU *  1 MCUs high per segment
                lines[channel] = ((lastSeg_l - firstSeg_l) / 14) * 8;
                image::Image ret(8, 1568, lines[channel], 1);

                std::set<uint32_t> bad_px;
                uint32_t index = 0;
                if (lastSeg_l != 0)
                    timestamps.clear();

                // Build the image from its constituent segments
                for (uint32_t x = firstSeg_l; x < lastSeg_l; x += 14)
                {
                    std::vector<double> line_timestamps;
                    for (uint32_t i = 0; i < 8; i++)
                    {
                        for (uint32_t j = 0; j < 14; j++)
                        {
                            if (segments[channel][x + j].isValid())
                            {
                                for (int f = 0; f < 14 * 8; f++)
                                    ret.set(index + f, segments[channel][x + j].lines[i][f]);

                                if (segments[channel][x + j].partial)
                                    for (uint32_t f = 0; f < 14 * 8; f++)
                                        bad_px.insert(index + f);

                                if (meteorm2x_mode)
                                    line_timestamps.push_back(segments[channel][x + j].timestamp);
                                else
                                    line_timestamps.push_back(dayValue + segments[channel][x + j].timestamp - 3 * 3600);
                            }
                            else
                            {
                                for (uint32_t f = 0; f < 14 * 8; f++)
                                {
                                    ret.set(index + f, 0);
                                    bad_px.insert(index + f);
                                }
                            }

                            index += 14 * 8;
                        }
                    }

                    timestamps.push_back(most_common(line_timestamps.begin(), line_timestamps.end(), -1.0));
                }

                // Fill missing data, if requested
                if (max_correct > 0 && ret.size() > 0)
                {
                    logger->info("Filling missing data in channel %d...", channel + 1);

                    // Interpolate image
                    size_t width = ret.width();
                    size_t height = ret.height();
#pragma omp parallel for
                    for (int x = 0; x < (int)width; x++)
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
                                        uint8_t top_val = ret.get(last_good * width + x);
                                        uint8_t bottom_val = ret.get(y * width + x);
                                        for (size_t fix_y = 1; fix_y < range; fix_y++)
                                        {
                                            float percent = (float)fix_y / (float)range;
                                            ret.set((fix_y + last_good) * width + x, (1.0f - percent) * (float)top_val + percent * (float)bottom_val);
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
        } // namespace lrpt
    } // namespace msumr
} // namespace meteor