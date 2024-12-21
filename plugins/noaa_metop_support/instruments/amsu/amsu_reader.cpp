#include "amsu_reader.h"

#include "common/ccsds/ccsds_time.h"
#include "common/utils.h"
#include "resources.h"
#include "common/calibration.h"

namespace noaa_metop
{
    namespace amsu
    {
        AMSUReader::AMSUReader()
        {
            amsu_a1_dat.resize(1);
            amsu_a2_dat.resize(1);
        }

        AMSUReader::~AMSUReader()
        {
        }

        void AMSUReader::work_A1(uint8_t *buffer)
        {

            for (int i = 0; i < 1020; i += 34)
                for (int j = 0; j < 13; j++)
                    amsu_a1_dat[linesA1].channels[j][i / 34] = (buffer[i + 16 + 2 * j] << 8) | buffer[16 + i + 2 * j + 1];

            // calibration
            for (int c = 0; c < 26; c += 2)
                amsu_a1_dat[linesA1].calibration_views_A1[c / 2] = view_pair{static_cast<uint16_t>((((buffer[1188 + c] << 8) | buffer[1189 + c]) + ((buffer[1214 + c] << 8) | buffer[1215 + c])) / 2),
                                                                             static_cast<uint16_t>((((buffer[1036 + c] << 8) | buffer[1037 + c]) + ((buffer[1062 + c] << 8) | buffer[1063 + c])) / 2)};
            //  temperatures
            for (int n = 0; n < 90; n += 2)
                amsu_a1_dat[linesA1].temperature_counts_A1[n / 2] = (buffer[1088 + n] << 8) | buffer[1089 + n];
        }

        void AMSUReader::work_A2(uint8_t *buffer)
        {
            for (int i = 0; i < 240; i += 8)
            {
                amsu_a2_dat[linesA2].channels[0][i / 8] = buffer[i + 12] << 8 | buffer[13 + i];
                amsu_a2_dat[linesA2].channels[1][i / 8] = buffer[i + 14] << 8 | buffer[14 + i];
            }

            // calibration
            for (int c = 0; c < 4; c += 2)
                amsu_a2_dat[linesA2].calibration_views_A2[c / 2] = view_pair{static_cast<uint16_t>((((buffer[304 + c] << 8) | buffer[305 + c]) + ((buffer[308 + c] << 8) | buffer[309 + c])) / 2),
                                                                             static_cast<uint16_t>((((buffer[252 + c] << 8) | buffer[253 + c]) + ((buffer[256 + c] << 8) | buffer[257 + c])) / 2)};
            // temperatures
            for (int n = 0; n < 38; n += 2)
                amsu_a2_dat[linesA2].temperature_counts_A2[n / 2] = (buffer[260 + n] << 8) | buffer[261 + n];
        }

        void AMSUReader::work_noaa(uint8_t *buffer)
        {
            uint8_t lines_since_timestamp = buffer[5] & 3;

            if (lines_since_timestamp > 3)
                lines_since_timestamp = 1;

            std::vector<uint8_t> amsuA2words;
            std::vector<uint8_t> amsuA1words;

            for (int j = 0; j < 14; j += 2)
            {
                if ((buffer[j + 35] % 2 != 1) || (buffer[j + 34] == 0xFF) || (buffer[j + 35] == 0xFF))
                {
                    amsuA2words.push_back(buffer[j + 34]);
                    amsuA2words.push_back(buffer[j + 35]);
                }
            }
            for (int j = 0; j < 26; j += 2)
            {
                if ((buffer[j + 9] % 2 != 1) || (buffer[j + 8] == 0xFF) || (buffer[j + 9] == 0xFF))
                {
                    amsuA1words.push_back(buffer[j + 8]);
                    amsuA1words.push_back(buffer[j + 9]);
                }
            }

            std::vector<std::vector<uint8_t>> amsuA2Data = amsuA2Deframer.work(amsuA2words.data(), amsuA2words.size());
            std::vector<std::vector<uint8_t>> amsuA1Data = amsuA1Deframer.work(amsuA1words.data(), amsuA1words.size());

            for (std::vector<uint8_t> frame : amsuA2Data)
            {
                work_A2(frame.data());
                if (contains(amsu_a2_dat, last_TIP_timestamp + (last_TIP_timestamp != -1 ? 8 * lines_since_timestamp : 0)))
                    amsu_a2_dat[linesA2].timestamp = -1;
                else
                    amsu_a2_dat[linesA2].timestamp = last_TIP_timestamp + (last_TIP_timestamp != -1 ? 8 * lines_since_timestamp : 0);
                amsu_a2_dat.resize(amsu_a2_dat.size() + 1);
                linesA2++;
            }

            for (std::vector<uint8_t> frame : amsuA1Data)
            {
                work_A1(frame.data());
                if (contains(amsu_a1_dat, last_TIP_timestamp + (last_TIP_timestamp != -1 ? 8 * lines_since_timestamp : 0)))
                    amsu_a1_dat[linesA1].timestamp = -1;
                else
                    amsu_a1_dat[linesA1].timestamp = last_TIP_timestamp + (last_TIP_timestamp != -1 ? 8 * lines_since_timestamp : 0);
                amsu_a1_dat.resize(amsu_a1_dat.size() + 1);
                linesA1++;
            }
        }

        void AMSUReader::work_metop(ccsds::CCSDSPacket &packet)
        {
            if (packet.header.apid == 39)
            {
                if (packet.payload.size() != 2096)
                    return;
                std::vector<uint8_t> filtered;
                for (unsigned int i = 13; i < packet.payload.size() - 2; i += 2)
                {
                    uint16_t word = (packet.payload[i + 1] << 8) | packet.payload[i + 2];
                    if (word != 1)
                    {
                        filtered.push_back(word >> 8);
                        filtered.push_back(word & 0xFF);
                    }
                }
                work_A1(filtered.data());
                amsu_a1_dat[linesA1].timestamp = ccsds::parseCCSDSTimeFull(packet, 10957);
                amsu_a1_dat.resize(amsu_a1_dat.size() + 1);
                linesA1++;
            }
            else if (packet.header.apid == 40)
            {
                if (packet.payload.size() != 1136)
                    return;
                std::vector<uint8_t> filtered;
                for (unsigned int i = 13; i < packet.payload.size() - 2; i += 2)
                {
                    uint16_t word = (packet.payload[i + 1] << 8) | packet.payload[i + 2];
                    if (word != 1)
                    {
                        filtered.push_back(word >> 8);
                        filtered.push_back(word & 0xFF);
                    }
                }
                work_A2(filtered.data());
                amsu_a2_dat[linesA2].timestamp = ccsds::parseCCSDSTimeFull(packet, 10957);
                amsu_a2_dat.resize(amsu_a2_dat.size() + 1);
                linesA2++;
            }
        }

        void AMSUReader::calibrate(nlohmann::json calib_coefs)
        {
            calib = calib_coefs;
            // calib_out["lua"] = loadFileToString(resources::getResourcePath("calibration/MHS.lua"));
            calib_out["calibrator"] = "noaa_mhs";

            double temps_A1[45];
            double temps_A2[19];

            bool PLL = calib["A1"]["PLL"].get<bool>();

            // ##############
            // ##### A1 #####
            // ##############
            for (int l = 0; l < linesA1; l++)
            {
                for (int i = 0; i < 45; i++)
                {
                    temps_A1[i] = 0;
                    for (int j = 0; j < 4; j++)
                    {
                        uint16_t count = amsu_a1_dat[l].temperature_counts_A1[i];
                        temps_A1[i] += calib["A1"]["instrument_temp_coefs"][i][j].get<double>() * pow(count / 2, j);
                    }
                }

                double Tw1 = 0;
                double Tw2 = 0;

                int sum1 = 0;
                int sum2 = 0;
                for (int i = 0; i < 5; i++)
                {
                    Tw1 += (temps_A1[i + 35] * calib["A1-1"]["wf"][i].get<int>());
                    sum1 += calib["A1-1"]["wf"][i].get<int>();

                    Tw2 += (temps_A1[i + 40] * calib["A1-2"]["wf"][i].get<int>());
                    sum2 += calib["A1-2"]["wf"][i].get<int>();
                }
                Tw1 /= sum1;
                Tw2 /= sum2;

                nlohmann::json ln;

                for (int c = 0; c < 13; c++)
                {
                    int index = calib["all"]["module"][c + 2].get<std::string>() == "A1-1" ? (PLL ? 2 : calib["A1"]["instrument_temerature_sensor_backup"].get<bool>()) : calib["A1"]["instrument_temerature_sensor_backup"].get<bool>();
                    double Rw = temperature_to_radiance((calib["all"]["module"][c + 2].get<std::string>() == "A1-1" ? Tw1 : Tw2) + extrapolate(
                                                                                                                                       {calib[calib["all"]["module"][c + 2].get<std::string>()]["u_temps"][index][0].get<double>(), calib["A1"]["warm_corr"][PLL][0][c].get<double>()},
                                                                                                                                       {calib[calib["all"]["module"][c + 2].get<std::string>()]["u_temps"][index][1].get<double>(), calib["A1"]["warm_corr"][PLL][1][c].get<double>()},
                                                                                                                                       {calib[calib["all"]["module"][c + 2].get<std::string>()]["u_temps"][index][2].get<double>(), calib["A1"]["warm_corr"][PLL][2][c].get<double>()},
                                                                                                                                       temps_A1[calib["A1"]["instrument_temerature_sensor_id"][calib["A1"]["instrument_temerature_sensor_backup"].get<bool>()].get<int>()]),
                                                        calib["all"]["wavenumber"][c + 2].get<double>());

                    double Rc = temperature_to_radiance(2.73 + calib["A1"]["cold_corr"][PLL][c].get<double>(), calib["all"]["wavenumber"][c + 2].get<double>());

                    double G = (amsu_a1_dat[l].calibration_views_A1[c].blackbody - amsu_a1_dat[l].calibration_views_A1[c].space) / (Rw - Rc);

                    double u = extrapolate(
                        {calib[calib["all"]["module"][c + 2].get<std::string>()]["u_temps"][index][0].get<double>(), calib["A1"]["u"][PLL][0][c].get<double>()},
                        {calib[calib["all"]["module"][c + 2].get<std::string>()]["u_temps"][index][1].get<double>(), calib["A1"]["u"][PLL][1][c].get<double>()},
                        {calib[calib["all"]["module"][c + 2].get<std::string>()]["u_temps"][index][2].get<double>(), calib["A1"]["u"][PLL][2][c].get<double>()},
                        temps_A1[calib["A1"]["instrument_temerature_sensor_id"][calib["A1"]["instrument_temerature_sensor_backup"].get<bool>()].get<int>()]);

                    ln[c + 2]["a0"] = Rw - (amsu_a1_dat[l].calibration_views_A1[c].blackbody / G) + u * (amsu_a1_dat[l].calibration_views_A1[c].blackbody * amsu_a1_dat[l].calibration_views_A1[c].space) / (G * G);
                    ln[c + 2]["a1"] = 1 / G - u * (amsu_a1_dat[l].calibration_views_A1[c].blackbody + amsu_a1_dat[l].calibration_views_A1[c].space) / (G * G);
                    ln[c + 2]["a2"] = u / (G * G);
                }

                calib_out["vars"]["perLine_perChannel"].push_back(ln);
            }
            // ##############
            // ##### A1 #####
            // ##############

            // ##############
            // ##### A2 #####
            // ##############
            for (int l = 0; l < linesA2; l++)
            {
                for (int i = 0; i < 19; i++)
                {
                    temps_A2[i] = 0;
                    for (int j = 0; j < 4; j++)
                    {
                        uint16_t count = amsu_a2_dat[l].temperature_counts_A2[i];
                        temps_A2[i] += calib["A2"]["instrument_temp_coefs"][i][j].get<double>() * pow(count / 2, j);
                    }
                }

                double Tw1 = 0;

                int sum1 = 0;
                for (int i = 0; i < 7; i++)
                {
                    Tw1 += temps_A2[i + 12] * calib["A2"]["wf"][i].get<int>();
                    sum1 += calib["A2"]["wf"][i].get<int>();
                }
                Tw1 /= sum1;

                for (int c = 0; c < 2; c++)
                {
                    double Rw = temperature_to_radiance(Tw1 + extrapolate(
                                                                  {calib["A2"]["u_temps"][calib["A2"]["instrument_temerature_sensor_backup"].get<bool>()][0].get<double>(), calib["A2"]["warm_corr"][0][c].get<double>()},
                                                                  {calib["A2"]["u_temps"][calib["A2"]["instrument_temerature_sensor_backup"].get<bool>()][1].get<double>(), calib["A2"]["warm_corr"][1][c].get<double>()},
                                                                  {calib["A2"]["u_temps"][calib["A2"]["instrument_temerature_sensor_backup"].get<bool>()][2].get<double>(), calib["A2"]["warm_corr"][2][c].get<double>()},
                                                                  temps_A2[calib["A2"]["instrument_temerature_sensor_id"][calib["A2"]["instrument_temerature_sensor_backup"].get<bool>()].get<int>()]),
                                                        calib["all"]["wavenumber"][c].get<double>());

                    double Rc = temperature_to_radiance(2.73 + calib["A2"]["cold_corr"][c].get<double>(), calib["all"]["wavenumber"][c].get<double>());

                    double G = (amsu_a2_dat[l].calibration_views_A2[c].blackbody - amsu_a2_dat[l].calibration_views_A2[c].space) / (Rw - Rc);

                    double u = extrapolate(
                        {calib["A2"]["u_temps"][calib["A2"]["instrument_temerature_sensor_backup"].get<bool>()][0].get<double>(), calib["A2"]["u"][0][c].get<double>()},
                        {calib["A2"]["u_temps"][calib["A2"]["instrument_temerature_sensor_backup"].get<bool>()][1].get<double>(), calib["A2"]["u"][1][c].get<double>()},
                        {calib["A2"]["u_temps"][calib["A2"]["instrument_temerature_sensor_backup"].get<bool>()][2].get<double>(), calib["A2"]["u"][2][c].get<double>()},
                        temps_A2[calib["A2"]["instrument_temerature_sensor_id"][calib["A2"]["instrument_temerature_sensor_backup"].get<bool>()].get<int>()]);

                    calib_out["vars"]["perLine_perChannel"][l][c]["a0"] = Rw - (amsu_a2_dat[l].calibration_views_A2[c].blackbody / G) + u * (amsu_a2_dat[l].calibration_views_A2[c].blackbody * amsu_a2_dat[l].calibration_views_A2[c].space) / (G * G);
                    calib_out["vars"]["perLine_perChannel"][l][c]["a1"] = 1 / G - u * (amsu_a2_dat[l].calibration_views_A2[c].blackbody + amsu_a2_dat[l].calibration_views_A2[c].space) / (G * G);
                    calib_out["vars"]["perLine_perChannel"][l][c]["a2"] = u / (G * G);
                }
            }
            // ##############
            // ##### A2 #####
            // ##############

            calib_out["wavenumbers"] = calib["all"]["wavenumber"];
        }

        void AMSUReader::correlate()
        {
            std::vector<double> all_timestamps;

            for (auto &f : amsu_a1_dat)
            {
                bool contains = false;
                for (auto &t : all_timestamps)
                    if (t == f.timestamp)
                        contains = true;
                if (!contains)
                    all_timestamps.push_back(f.timestamp);
            }

            for (auto &f : amsu_a2_dat)
            {
                bool contains = false;
                for (auto &t : all_timestamps)
                    if (t == f.timestamp)
                        contains = true;
                if (!contains)
                    all_timestamps.push_back(f.timestamp);
            }

            std::sort(all_timestamps.begin(), all_timestamps.end());

            std::vector<a1_data_t> amsu_a1_ndat;
            std::vector<a2_data_t> amsu_a2_ndat;

            for (auto &t : all_timestamps)
            {
                bool had = false;
                for (auto &f : amsu_a1_dat)
                    if (f.timestamp == t)
                    {
                        amsu_a1_ndat.push_back(f);
                        had = true;
                    }
                if (!had)
                    amsu_a1_ndat.push_back(a1_data_t());

                had = false;
                for (auto &f : amsu_a2_dat)
                    if (f.timestamp == t)
                    {
                        amsu_a2_ndat.push_back(f);
                        had = true;
                    }
                if (!had)
                    amsu_a2_ndat.push_back(a2_data_t());
            }

            amsu_a1_dat = amsu_a1_ndat;
            amsu_a2_dat = amsu_a2_ndat;
            common_timestamps = all_timestamps;
            linesA1 = amsu_a1_dat.size();
            linesA2 = amsu_a2_dat.size();
        }

        image::Image AMSUReader::getChannel(int channel)
        {
            image::Image img(16, 30, (channel < 2 ? linesA2 : linesA1), 1);
            if (channel < 2)
                for (int l = 0; l < linesA2; l++)
                    for (int c = 0; c < 30; c++)
                        img.set(l * 30 + c, amsu_a2_dat[l].channels[channel][c]);
            else
                for (int l = 0; l < linesA1; l++)
                    for (int c = 0; c < 30; c++)
                        img.set(l * 30 + c, amsu_a1_dat[l].channels[channel - 2][c]);
            img.mirror(true, false);
            return img;
        }
    }
}