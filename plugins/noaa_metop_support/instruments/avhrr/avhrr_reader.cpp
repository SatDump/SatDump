#include "avhrr_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

namespace noaa_metop
{
    namespace avhrr
    {
        AVHRRReader::AVHRRReader(bool gac) : gac_mode(gac), width(gac_mode ? 409 : 2048)
        {
            for (int i = 0; i < 6; i++)
                channels[i].resize(width);
            lines = 0;
        }

        AVHRRReader::~AVHRRReader()
        {
            for (int i = 0; i < 5; i++)
                channels[i].clear();
            prt_buffer.clear();
            views.clear();
        }

        void AVHRRReader::work_metop(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 12960)
                return;

            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            // Convert AVHRR payload into 10-bits values
            repackBytesTo10bits(&packet.payload[14], 12944, avhrr_buffer);

            line2image(avhrr_buffer, 55, 2048, packet.header.apid == 103);
        }

        void AVHRRReader::work_noaa(uint16_t *buffer)
        {
            // Parse timestamp
            int day_of_year = buffer[8] >> 1;
            uint64_t milliseconds = (buffer[9] & 0x7F) << 20 | (buffer[10] << 10) | buffer[11];
            timestamps.push_back(ttp.getTimestamp(day_of_year, milliseconds));

            int pos = gac_mode ? 1182 : 750; // AVHRR Data
            line2image(buffer, pos, width, buffer[6] & 1);
        }

        image::Image<uint16_t> AVHRRReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].data(), width, lines, 1);
        }

        void AVHRRReader::line2image(uint16_t *buff, int pos, int width, bool is_ch3a)
        {
            for (int channel = 0; channel < 5; channel++)
                for (int i = 0; i < width; i++)
                {
                    int ch = channel;
                    if (is_ch3a)
                    {
                        if (channel > 2)
                            ch++;
                    }
                    else if (channel > 1)
                        ch++;

                    channels[ch][lines * width + i] = buff[pos + channel + i * 5] << 6;
                }

            // Frame counter
            lines++;

            for (int channel = 0; channel < 6; channel++)
                channels[channel].resize((lines + 1) * 2048);

            // calibration data extraction (for later, we don't know what sat this is yet!)
            prt_buffer.push_back((buff[17] + buff[18] + buff[19])/3);

            // AVHRR has space data for all 5 channels, but it will be not used for VIS in this implementation, so can be ommited
            uint16_t avg_blb[3] = {0}; // blackbody average
            uint16_t avg_spc[3] = {0}; // space average
            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 3; j++)
                {
                    avg_blb[j] += buff[22 + 3 * i + j];
                    avg_spc[j] += buff[52 + 5 * i + j + 2];
                }
            for (int j = 0; j < 3; j++){
                avg_blb[j] /= 10;
                avg_spc[j] /= 10;
            }

            std::array<view_pair, 3> el;
            for (int i = 0; i < 3; i++){
                el[i] = view_pair{avg_spc[i], avg_blb[i]};
            }
            views.push_back(el);
        }

        int AVHRRReader::calibrate(std::string sat_name){ // further calibration
            
        }
    } // namespace avhrr
} // namespace metop_noaa