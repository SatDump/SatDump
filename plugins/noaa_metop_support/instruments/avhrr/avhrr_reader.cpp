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

            if (!gac_mode) // we don't have info on GAC rn, we'll have to RE
            {
                // calibration data extraction (for later, we don't know what sat this is yet!)
                prt_buffer.push_back(buff[17] * buff[18] * buff[19] == 0 ? 0 : (buff[17] + buff[18] + buff[19]) / 3);
                // prt_buffer.push_back((buff[17] + buff[18] + buff[19]) / 3);

                // AVHRR has space data for all 5 channels, but it will be not used for VIS in this implementation, so can be ommited
                uint16_t avg_blb[3] = {0}; // blackbody average
                uint16_t avg_spc[3] = {0}; // space average
                for (int i = 0; i < 10; i++)
                    for (int j = 0; j < 3; j++)
                    {
                        avg_blb[j] += buff[22 + 3 * i + j];
                        avg_spc[j] += buff[52 + 5 * i + j + 2];
                    }
                for (int j = 0; j < 3; j++)
                {
                    avg_blb[j] /= 10;
                    avg_spc[j] /= 10;
                }

                std::array<view_pair, 3> el;
                for (int i = 0; i < 3; i++)
                {
                    el[i] = view_pair{avg_spc[i], avg_blb[i]};
                }
                views.push_back(el);
            }
        }

        int AVHRRReader::calibrate(nlohmann::json calib_coefs)
        { // further calibration
            // read params
            std::vector<std::vector<double>> prt_coefs = calib_coefs["PRT"].get<std::vector<std::vector<double>>>();
            std::vector<std::map<std::string, double>> channel_coefs;
            for (int i = 0; i < 6; i++)
                channel_coefs[i] = calib_coefs[std::to_string(i)].get<std::map<std::string, double>>();

            for (int i = 0; i < prt_buffer.size(); i++)
            {
                // PRT counts to temperature but NOAA made it annoying
                if (i >= 4 && prt_buffer[i] == 0)
                {
                    uint16_t tbb = 0;
                    int chk = 0;
                    for (int n = 1; n <= 4; n++)
                    {
                        if (!prt_buffer[i - n] == 0)
                        {
                            for (int p = 0; p < 5; p++)
                                tbb += prt_coefs[4 - n][p] * pow(prt_buffer[i - n], p);
                            chk++;
                        }
                        else
                            break;
                    }
                    tbb /= chk;
                    for (int n = 1; n <= 4; n++)
                        for (int c = 3; c < 6; c++){ // replace chk + 1 lines from [i-1]
                            //struct.channel.nbb = temperature_to_radiance(channel_coefs[c]["A"] + channel_coefs[c]["B"] * tbb, channel_coefs[c]["wavnb"]);
                        }
                    
                }
                //per_line_struct.prt_temp.all_ch.push_back(-1);
                

                //
            }
        }
    } // namespace avhrr
} // namespace metop_noaa