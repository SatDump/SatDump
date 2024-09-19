#include "avhrr_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "common/utils.h"

namespace noaa_metop
{
    namespace avhrr
    {
        AVHRRReader::AVHRRReader(bool gac, int year) : gac_mode(gac), width(gac_mode ? 409 : 2048), ttp(year)
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
                // prt_buffer.push_back(prts[2] + prts[3] + prts[4] < 20 ? 0 : (prts[2] + prts[3] + prts[4]) / 3);

                uint8_t prts_cnt = 0;
                uint16_t prts_avg = 0;
                for (int i = 0; i < 3; i++)
                {
                    if (prts[2 + i] > 100 && prts[2 + i] < 500)
                    { // limits check
                        prts_avg += prts[2 + i];
                        prts_cnt++;
                    }
                }
                if (prts_cnt != 0)
                    prt_buffer.push_back(prts_avg / prts_cnt);
                else
                    prt_buffer.push_back(0);

                // AVHRR has space data for all 5 channels, but it will be not used for VIS in this implementation, so can be ommited
                uint16_t avg_blb[3] = {0, 0, 0}; // blackbody average
                uint16_t avg_spc[3] = {0, 0, 0}; // space average

                std::vector<uint16_t> blb_list[3];
                std::vector<uint16_t> spc_list[3];

                for (int i = 0; i < 10; i++)
                    for (int j = 0; j < 3; j++)
                    {
                        if (avhrr_buffer[10305 + 5 * i + j + 2] > blb_limits[j])
                        {
                            avg_blb[j] += avhrr_buffer[10305 + 5 * i + j + 2];
                            blb_list[j].push_back(avhrr_buffer[10305 + 5 * i + j + 2]);
                        }
                        if (avhrr_buffer[0 + 5 * i + j + 2] > 880)
                        {
                            avg_spc[j] += avhrr_buffer[0 + 5 * i + j + 2];
                            spc_list[j].push_back(avhrr_buffer[0 + 5 * i + j + 2]);
                        }
                    }
                for (int j = 0; j < 3; j++)
                {
                    if (blb_list[j].size() == 0)
                        continue;

                    avg_blb[j] /= blb_list[j].size();
                    for (uint8_t k = 0; k < blb_list[j].size(); k++)
                    {
                        if (abs(blb_list[j][k] - avg_blb[j]) > BLB_LIMIT)
                        {
                            blb_list[j].erase(blb_list[j].begin() + k);
                            k--;
                        }
                    }
                    if (blb_list[j].size() != 0)
                    {
                        avg_blb[j] = 0;
                        for (uint8_t k = 0; k < blb_list[j].size(); k++)
                        {
                            avg_blb[j] += blb_list[j][k];
                        }

                        avg_blb[j] /= blb_list[j].size();
                    }
                    if (spc_list[j].size() == 0)
                        continue;
                    avg_spc[j] /= spc_list[j].size();
                    for (uint8_t k = 0; k < spc_list[j].size(); k++)
                    {
                        if (abs(spc_list[j][k] - avg_spc[j]) > SPC_LIMIT)
                        {
                            spc_list[j].erase(spc_list[j].begin() + k);
                            k--;
                        }
                    }
                    if (spc_list[j].size() != 0)
                    {
                        avg_spc[j] = 0;
                        for (uint8_t k = 0; k < spc_list[j].size(); k++)
                        {
                            avg_spc[j] += spc_list[j][k];
                        }
                        avg_spc[j] /= spc_list[j].size();
                    }
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
            // prt_buffer.push_back(buffer[17] * buffer[18] * buffer[19] == 0 ? 0 : (buffer[17] + buffer[18] + buffer[19]) / 3);
            // prt_buffer.push_back((buff[17] + buff[18] + buff[19]) / 3);

            uint16_t *prts = &buffer[17];
            uint8_t prts_cnt = 0;
            uint16_t prts_avg = 0;
            for (int i = 0; i < 3; i++)
            {
                if (prts[i] > 100 && prts[i] < 500)
                { // limits check
                    prts_avg += prts[i];
                    prts_cnt++;
                }
            }
            if (prts_cnt != 0)
                prt_buffer.push_back(prts_avg / prts_cnt);
            else
                prt_buffer.push_back(0);

            // AVHRR has space data for all 5 channels, but it will be not used for VIS in this implementation, so can be ommited
            uint16_t avg_blb[3] = {0, 0, 0}; // blackbody average
            uint16_t avg_spc[3] = {0, 0, 0}; // space average

            std::vector<uint16_t> blb_list[3];
            std::vector<uint16_t> spc_list[3];

            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 3; j++)
                {
                    if (buffer[22 + 3 * i + j] > blb_limits[j])
                    {
                        avg_blb[j] += buffer[22 + 3 * i + j];
                        blb_list[j].push_back(buffer[22 + 3 * i + j]);
                    }
                    if (buffer[52 + 5 * i + j + 2] > 880)
                    {
                        avg_spc[j] += buffer[52 + 5 * i + j + 2];
                        spc_list[j].push_back(buffer[52 + 5 * i + j + 2]);
                    }
                }
            for (int j = 0; j < 3; j++)
            {
                if (blb_list[j].size() == 0)
                    continue;
                avg_blb[j] /= blb_list[j].size();
                for (uint8_t k = 0; k < blb_list[j].size(); k++)
                {
                    if (abs(blb_list[j][k] - avg_blb[j]) > BLB_LIMIT)
                    {
                        blb_list[j].erase(blb_list[j].begin() + k);
                        k--;
                    }
                }
                if (blb_list[j].size() != 0)
                {
                    avg_blb[j] = 0;
                    for (uint8_t k = 0; k < blb_list[j].size(); k++)
                    {
                        avg_blb[j] += blb_list[j][k];
                    }

                    avg_blb[j] /= blb_list[j].size();
                }
                if (spc_list[j].size() == 0)
                    continue;
                avg_spc[j] /= spc_list[j].size();
                for (uint8_t k = 0; k < spc_list[j].size(); k++)
                {
                    if (abs(spc_list[j][k] - avg_spc[j]) > SPC_LIMIT)
                    {
                        spc_list[j].erase(spc_list[j].begin() + k);
                        k--;
                    }
                }
                if (spc_list[j].size() != 0)
                {
                    avg_spc[j] = 0;
                    for (uint8_t k = 0; k < spc_list[j].size(); k++)
                    {
                        avg_spc[j] += spc_list[j][k];
                    }
                    avg_spc[j] /= spc_list[j].size();
                }
            }

            std::array<view_pair, 3> el;
            for (int i = 0; i < 3; i++)
            {
                el[i] = view_pair{avg_spc[i], avg_blb[i]};
            }
            views.push_back(el);
        }

        image::Image AVHRRReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, width, lines, 1);
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
                calib_out["vars"]["perChannel"][p] = calib_coefs["channels"][p];
            }
            for (int p = 0; p < 6; p++)
                calib_out["wavenumbers"][p] = calib_coefs["channels"][p]["wavnb"].get<double>();

            // calib_out["lua"] = loadFileToString(resources::getResourcePath("calibration/AVHRR.lua"));
            calib_out["calibrator"] = "noaa_avhrr3";

            double ltbb = -1;
            std::vector<nlohmann::json> ln;
            ln.resize(6);
            for (unsigned int i = 0; i < prt_buffer.size(); i++)
            {
                // PRT counts to temperature but NOAA made it annoying
                if (i > 4 && prt_buffer[i] == 0)
                {
                    double tbb = 0;
                    int chk = 1;
                    std::array<view_pair, 3> avgpair;
                    do
                    {

                        for (int p = 0; p < 4; p++)
                        {
                            tbb += prt_coefs[4 - chk][p] * pow(prt_buffer[i - chk], p); // convert PRT counts to temperature
                        }
                        for (int p = 0; p < 3; p++)
                            avgpair[p] += views[i - chk][p]; // average the views

                        chk++;
                    } while (chk <= 4 && prt_buffer[i - chk] != 0);

                    tbb /= (chk - 1) == 0 ? 1 : (chk - 1);
                    for (int p = 0; p < 3; p++)
                        avgpair[p] /= (chk - 1) == 0 ? 1 : (chk - 1);
                    ltbb = tbb;
                    for (int n = 1; n <= chk; n++)
                    {
                        for (int c = 3; c < 6; c++)
                        { // replace chk + 1 lines from [i] back
                            calib_out["vars"]["perLine_perChannel"][i - n][c]["Spc"] = avgpair[c - 3].space;
                            calib_out["vars"]["perLine_perChannel"][i - n][c]["Blb"] = avgpair[c - 3].blackbody;
                            calib_out["vars"]["perLine_perChannel"][i - n][c]["Nbb"] =
                                temperature_to_radiance(calib_coefs["channels"][c]["A"].get<double>() + calib_coefs["channels"][c]["B"].get<double>() * tbb,
                                                        calib_coefs["channels"][c]["Vc"].get<double>());
                        }
                    }
                }
                for (int c = 3; c < 6; c++)
                {
                    if (calib_out["vars"]["perLine_perChannel"].size() > 0)
                        if (calib_out["vars"]["perLine_perChannel"][calib_out["vars"]["perLine_perChannel"].size() - 1][c].contains("Spc"))
                        {
                            ln[c]["Spc"] = calib_out["vars"]["perLine_perChannel"][calib_out["vars"]["perLine_perChannel"].size() - 1][c]["Spc"];
                            ln[c]["Blb"] = calib_out["vars"]["perLine_perChannel"][calib_out["vars"]["perLine_perChannel"].size() - 1][c]["Blb"];
                        }
                    ln[c]["Nbb"] =
                        temperature_to_radiance(calib_coefs["channels"][c]["A"].get<double>() + calib_coefs["channels"][c]["B"].get<double>() * ltbb,
                                                calib_coefs["channels"][c]["Vc"].get<double>());
                    ln[c]["Ns"] = calib_coefs["channels"][c]["Ns"].get<double>();
                    ln[c]["b"] = calib_coefs["channels"][c]["b"].get<std::vector<double>>();
                }
                calib_out["vars"]["perLine_perChannel"].push_back(ln);
            }
        }
    } // namespace avhrr
} // namespace metop_noaa