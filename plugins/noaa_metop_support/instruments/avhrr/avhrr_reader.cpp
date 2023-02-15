#include "avhrr_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "common/utils.h"

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

            {
                uint16_t *prts = &avhrr_buffer[10295];
                prt_buffer.push_back(prts[2] * prts[3] * prts[4] == 0 ? 0 : (prts[2] + prts[3] + prts[4]) / 3);

                // AVHRR has space data for all 5 channels, but it will be not used for VIS in this implementation, so can be ommited
                uint16_t avg_blb[3] = {0, 0, 0}; // blackbody average
                uint16_t avg_spc[3] = {0, 0, 0}; // space average
                for (int i = 0; i < 10; i++)
                    for (int j = 0; j < 3; j++)
                    {
                        avg_blb[j] += avhrr_buffer[10305 + 5 * i + j + 2];
                        avg_spc[j] += avhrr_buffer[0 + 5 * i + j + 2];
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

        void AVHRRReader::work_noaa(uint16_t *buffer)
        {
            // Parse timestamp
            int day_of_year = buffer[8] >> 1;
            uint64_t milliseconds = (buffer[9] & 0x7F) << 20 | (buffer[10] << 10) | buffer[11];
            timestamps.push_back(ttp.getTimestamp(day_of_year, milliseconds));

            int pos = gac_mode ? 1182 : 750; // AVHRR Data
            line2image(buffer, pos, width, buffer[6] & 1);

            // calibration data extraction (for later, we don't know what sat this is yet!)
            prt_buffer.push_back(buffer[17] * buffer[18] * buffer[19] == 0 ? 0 : (buffer[17] + buffer[18] + buffer[19]) / 3);
            // prt_buffer.push_back((buff[17] + buff[18] + buff[19]) / 3);

            // AVHRR has space data for all 5 channels, but it will be not used for VIS in this implementation, so can be ommited
            uint16_t avg_blb[3] = {0, 0, 0}; // blackbody average
            uint16_t avg_spc[3] = {0, 0, 0}; // space average
            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 3; j++)
                {
                    avg_blb[j] += buffer[22 + 3 * i + j];
                    avg_spc[j] += buffer[52 + 5 * i + j + 2];
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
        }

        void AVHRRReader::calibrate(nlohmann::json calib_coefs)
        { // further calibration
            // read params
            std::vector<std::vector<double>> prt_coefs = calib_coefs["PRT"].get<std::vector<std::vector<double>>>();

            for (int p = 0; p < 6; p++)
            {
                calib_out["lua_vars"]["perChannel"][p] = calib_coefs["channels"][p];
            }
            for (int p = 0; p < 6; p++)
                calib_out["wavenumbers"][p] = calib_coefs["channels"][p]["wavnb"].get<double>();

            calib_out["lua"] = loadFileToString(resources::getResourcePath("calibration/AVHRR.lua"));

            double ltbb = -1;
            std::vector<nlohmann::json> ln;
            ln.resize(6);
            for (unsigned int i = 0; i < prt_buffer.size(); i++)
            {
                // PRT counts to temperature but NOAA made it annoying
                if (i >= 4 && prt_buffer[i] == 0)
                {
                    double tbb = 0;
                    int chk = 0;
                    std::array<view_pair, 3> avgpair;
                    for (int n = 1; n <= 4; n++)
                    {
                        if (!prt_buffer[i - n] == 0)
                        {
                            for (int p = 0; p < 5; p++)
                                tbb += prt_coefs[4 - n][p] * pow(prt_buffer[i - n], p); // convert PRT counts to temperature

                            for (int p = 0; p < 3; p++)
                                avgpair[p] += views[i - n][p]; // average the views

                            chk++;
                        }
                        else
                            break;
                    }
                    tbb /= chk != 0 ? chk : 1;
                    for (int p = 0; p < 3; p++)
                        avgpair[p] /= chk != 0 ? chk : 1;
                    ltbb = tbb;
                    for (int n = 0; n <= chk; n++)
                        for (int c = 3; c < 6; c++)
                        { // replace chk + 1 lines from [i] back
                            calib_out["lua_vars"]["perLine_perChannel"][i - n][c]["Spc"] = avgpair[c - 3].space;
                            calib_out["lua_vars"]["perLine_perChannel"][i - n][c]["Blb"] = avgpair[c - 3].blackbody;
                            calib_out["lua_vars"]["perLine_perChannel"][i - n][c]["Nbb"] =
                                temperature_to_radiance(calib_coefs["channels"][c]["A"].get<double>() + calib_coefs["channels"][c]["B"].get<double>() * tbb,
                                                        calib_coefs["channels"][c]["Vc"].get<double>());
                        }
                }
                for (int c = 3; c < 6; c++)
                {
                    ln[c]["Nbb"] =
                        temperature_to_radiance(calib_coefs["channels"][c]["A"].get<double>() + calib_coefs["channels"][c]["B"].get<double>() * ltbb,
                                                calib_coefs["channels"][c]["Vc"].get<double>());
                    ln[c]["Ns"] = calib_coefs["channels"][c]["Ns"].get<double>();
                    ln[c]["b"] = calib_coefs["channels"][c]["b"].get<std::vector<double>>();
                }
                calib_out["lua_vars"]["perLine_perChannel"].push_back(ln);
            }
        }
    } // namespace avhrr
} // namespace metop_noaa