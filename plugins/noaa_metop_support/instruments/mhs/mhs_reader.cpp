/*

NOAA MHS decoder by ZbychuButItWasTaken

This decoder takes in raw AIP data and processes it to MHS. It perfprms calibration (based on NOAA KLM User's Guide) and rain detection (though not very well as N19's MHS is broken)

*/

#include "mhs_reader.h"
#include <cstring>
#include "resources.h"
#include "nlohmann/json_utils.h"
#include "common/utils.h"
#include "common/calibration.h"

#include "logger.h"

namespace noaa_metop
{
    namespace mhs
    {
        MHSReader::MHSReader()
        {
            std::memset(MIU_data, 0, 80 * 50);
            // deb_out.open("test.bin");
        }

        void MHSReader::work(uint8_t *buffer)
        {
            // deb_out.write((char *)buffer, SCI_PACKET_SIZE);
            std::array<std::array<uint16_t, MHS_WIDTH>, 5> linebuff;
            std::memset(&linebuff, 0, MHS_WIDTH * 5 * 2); // make some room

            for (int i = 0; i < MHS_WIDTH; i++)
            {
                for (int c = 1; c < 6; c++)
                {
                    linebuff[c - 1][i] = buffer[MHS_OFFSET + i * 12 + c * 2] << 8 | buffer[MHS_OFFSET + i * 12 + c * 2 + 1]; // extract image data
                }
            }
            for (int c = 0; c < 5; c++)
                channels[c].push_back(linebuff[c]);
            line++;

            // calib values

            // read the needed values for calibration from the SCI Packet
            calib_line cl;
            for (int i = 0; i < 3; i++)
                cl.PRT_calib[i] = (buffer[PRT_OFFSET + i * 2 + 10] << 8) | buffer[PRT_OFFSET + i * 2 + 11];

            for (int i = 0; i < 5; i++)
                cl.PRT_readings[i] = buffer[PRT_OFFSET + i * 2] << 8 | buffer[PRT_OFFSET + i * 2 + 1];

            for (int i = 0; i < 39; i++)
                cl.HK[i] = buffer[i];

            // get the calibration views from the blackbody and space
            for (int c = 0; c < 5; c++)
                for (int j = 0; j < 2; j++)
                    cl.calibration_views[c][j] = 0;

            for (int c = 0; c < 5; c++)
            {
                for (int j = 0; j < 2; j++)
                {
                    int avg = 0;                  //
                    std::vector<uint16_t> counts; //  temporary variables so we can check for gross error
                    for (int k = 0; k < 4; k++)
                    {
                        uint16_t sample = (buffer[MHS_OFFSET + (j * 4 + k + MHS_WIDTH) * 12 + (c + 1) * 2] << 8 | buffer[MHS_OFFSET + (j * 4 + k + MHS_WIDTH) * 12 + (c + 1) * 2 + 1]);
                        if (sample > limits[j][0] && sample < limits[j][1])
                        {
                            avg += sample;
                            counts.push_back(sample);
                        }
                    }
                    if (counts.size() == 0)
                        continue;
                    avg /= counts.size();
                    for (uint8_t k = 0; k < counts.size(); k++)
                    {
                        if (abs(counts[k] - avg) > CAL_LIMIT) // check for bad samples
                        {
                            counts.erase(counts.begin() + k);
                            k--;
                        }
                    }
                    if (counts.size() > 0)
                    {
                        avg = 0;
                        for (uint8_t k = 0; k < counts.size(); k++)
                        {
                            avg += counts[k];
                        }
                        avg /= counts.size();
                    }
                    cl.calibration_views[c][j] = avg; // finally push the counts
                }
            }

            PIE_buff.push_back((buffer[0] >> 3) & 1);

            calib_lines.push_back(cl);
        }

        image::Image MHSReader::getChannel(int channel)
        {
            image::Image output(16, MHS_WIDTH, line, 1);
            for (int l = 0; l < line; l++)
                for (int x = 0; x < MHS_WIDTH; x++)
                    output.set(l * MHS_WIDTH + (MHS_WIDTH - x - 1), channels[channel][l][x]);
            return output;
        }

        double MHSReader::get_u(double temp, int ch)
        {
            if (temp == calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][0].get<double>())
                return calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][0][ch].get<double>();

            if (temp == calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][1].get<double>())
                return calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][1][ch].get<double>();

            if (temp == calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][2].get<double>())
                return calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][2][ch].get<double>();

            if (temp < calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][0].get<double>())
                return interpolate(calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][0].get<double>(), calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][0][ch].get<double>(), calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][1].get<double>(), calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][1][ch].get<double>(), temp, 0);

            if (temp > calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][0].get<double>() && temp < calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][1].get<double>())
                return interpolate(calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][0].get<double>(), calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][0][ch].get<double>(), calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][1].get<double>(), calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][1][ch].get<double>(), temp, 0);

            if (temp > calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][1].get<double>() && temp < calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][2].get<double>())
                return interpolate(calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][1].get<double>(), calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][1][ch].get<double>(), calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][2].get<double>(), calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][2][ch].get<double>(), temp, 0);

            else
                return interpolate(calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][1].get<double>(), calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][1][ch].get<double>(), calib["u_temps"][calib["instrument_temerature_sensor_backup"].get<bool>()][2].get<double>(), calib["u"][calib["instrument_temerature_sensor_backup"].get<bool>()][2][ch].get<double>(), temp, 1);
        }

        double MHSReader::interpolate(double a1x, double a1y, double a2x, double a2y, double bx, int mode)
        {                  // where a1y > a2y and a1x < a1y
            if (mode == 0) // between a1 and a2 or bx < a1x
                return ((a2x - bx) * (a1y - a2y)) / (a2x - a1x) + a2y;
            else // > a2x
                return -1 * (((bx - a2x) * (a1y - a2y)) / (a2x - a1x) - a2y);
        }

        void MHSReader::calibrate(nlohmann::json calib_coefs)
        {
            // declare all the variables
            calib = calib_coefs;
            // calib_out["lua"] = loadFileToString(resources::getResourcePath("calibration/MHS.lua"));
            calib_out["calibrator"] = "noaa_mhs";

            uint8_t PIE = most_common(PIE_buff.begin(), PIE_buff.end(), 0);
            PIE_buff.clear();

            double a, b;
            double R[5], Tk[5], WTk = 0, Wk = 0, Tw;
            std::array<double, 24> Tth;                            

            // weighed average of the samples, as described by the NOAA KLM User's Guide equation 7.6.6-3
            std::vector<uint16_t> conv_views[5][2];
            for (int c = 0; c < 5; c++)
            {
                conv_views[c][0].resize(line);
                conv_views[c][1].resize(line);
                for (int i = 0; i < 3; i++)
                {
                    for (int j = 0; j < 2; j++)
                    {
                        conv_views[c][j][i] = calib_lines[i].calibration_views[c][j];
                        conv_views[c][j][line - i - 1] = calib_lines[line - i - 1].calibration_views[c][j];
                    }
                }
            }
            for (int l = 3; l < line - 3; l++)
            {
                for (int j = 0; j < 2; j++)
                {
                    for (int c = 0; c < 5; c++)
                    {
                        uint32_t avg = 0;
                        uint8_t wc = 0;
                        for (int pos = -3; pos <= 3; pos++)
                        {
                            if (calib_lines[l + pos].calibration_views[c][j] != 0)
                            {
                                avg += calib_lines[l + pos].calibration_views[c][j] * calib["avg_weights"][pos + 3].get<int>();
                                wc += calib["avg_weights"][pos + 3].get<int>();
                            }
                        }
                        if (wc != 0)
                            avg /= wc;
                        conv_views[c][j][l] = avg;
                    }
                }
            }

            for (int l = 0; l < line; l++)
            {
                // reset it all
                a = 0;
                b = 0;
                WTk = 0;
                Wk = 0;
                Tw = 0;

                for (int r = 0; r < 5; r++)
                {
                    R[r] = 0;
                    Tk[r] = 0;
                }

                double RCALn = calib["RCAL"][PIE][0].get<double>() + calib["RCAL"][PIE][1].get<double>() + calib["RCAL"][PIE][2].get<double>();

                // math for calculating PRT temperature
                double CCALn = calib_lines[l].PRT_calib[0] + calib_lines[l].PRT_calib[1] + calib_lines[l].PRT_calib[2];
                double C2CALn = pow((double)calib_lines[l].PRT_calib[0], 2.0) + pow((double)calib_lines[l].PRT_calib[1], 2.0) + pow((double)calib_lines[l].PRT_calib[2], 2.0);
                double RCCALn = (double)calib_lines[l].PRT_calib[0] * calib["RCAL"][PIE][0].get<double>() + (double)calib_lines[l].PRT_calib[1] * calib["RCAL"][PIE][1].get<double>() + (double)calib_lines[l].PRT_calib[2] * calib["RCAL"][PIE][2].get<double>();
                a = (RCALn * C2CALn - CCALn * RCCALn) / (3 * C2CALn - pow(CCALn, 2.0));
                b = (3 * RCCALn - RCALn * CCALn) / (3 * C2CALn - pow(CCALn, 2.0));

                for (int i = 0; i < 5; i++)
                {
                    R[i] = a + b * calib_lines[l].PRT_readings[i];
                    Tk[i] = 0;
                    for (int j = 0; j <= 3; j++)
                    {
                        Tk[i] += calib["f"][PIE][i][j].get<double>() * pow(R[i], (double)j);
                    }
                    if (Tk[i] > 260 && Tk[i] < 310) // check gross limits of PRT temp
                    {
                        WTk += (calib["W"][i].get<double>() * Tk[i]);
                        Wk += calib["W"][i].get<double>();
                    }
                }
                if (Wk != 0)
                {
                    Tw = WTk / Wk;
                    last_Tw = Tw;
                }

                // average instrument temperature
                for (int i = 0; i < 24; i++)
                {
                    Tth[i] = 0.0;
                    for (int j = 0; j <= 4; j++)
                    {
                        Tth[i] += (calib["g"][j].get<double>() * pow((double)calib_lines[l].HK[i + HKTH_offset], (double)j));
                    }
                }

                // calibration of each channel
                nlohmann::json ln;
                for (int i = 0; i < 5; i++)
                {
                    double Twp = calib["corr"][i][0].get<double>() + calib["corr"][i][1].get<double>() * Tw;
                    if (Tw == 0) // fill in missing lines (not optimal, should use an average, but good enough for our needs)
                        Twp = calib["corr"][i][0].get<double>() + calib["corr"][i][1].get<double>() * last_Tw;

                    if (Twp == 0) // disable calib for the line if no calib data is available
                    {
                        ln[i]["a0"] = -999.99;
                    }
                    else
                    {
#if 1
                        double G = (conv_views[i][1][l] - conv_views[i][0][l]) / (temperature_to_radiance(Twp, calib["wavenumber"][i].get<double>()) - temperature_to_radiance(2.73 + calib["cs_corr"][calib["cs_corr_id"].get<int>()][i].get<double>(), calib["wavenumber"][i].get<double>()));
                        ln[i]["a0"] = temperature_to_radiance(Twp, calib["wavenumber"][i].get<double>()) - (conv_views[i][1][l] / G) + get_u(Tth[calib["instrument_temerature_sensor_backup"].get<bool>() ? 3 : 0], i) * ((conv_views[i][1][l] * conv_views[i][0][l]) / pow(G, 2.0));
                        ln[i]["a1"] = 1.0 / G - get_u(Tth[calib["instrument_temerature_sensor_backup"].get<bool>() ? 3 : 0], i) * ((conv_views[i][0][l] + conv_views[i][1][l]) / pow(G, 2.0));
                        ln[i]["a2"] = get_u(Tth[calib["instrument_temerature_sensor_backup"].get<bool>() ? 3 : 0], i) * (1.0 / pow(G, 2.0));
#endif
#if 0
                        double G = (calib_lines[l].calibration_views[i][1] - calib_lines[l].calibration_views[i][0]) / (temperature_to_radiance(Twp, calib["wavenumber"][i].get<double>()) - temperature_to_radiance(2.73 + calib["cs_corr"][calib["cs_corr_id"].get<int>()][i].get<double>(), calib["wavenumber"][i].get<double>()));
                        ln[i]["a0"] = temperature_to_radiance(Twp, calib["wavenumber"][i].get<double>()) - (calib_lines[l].calibration_views[i][1] / G) + get_u(Tth[calib["instrument_temerature_sensor_backup"].get<bool>() ? 3 : 0], i) * ((calib_lines[l].calibration_views[i][1] * calib_lines[l].calibration_views[i][0]) / pow(G, 2.0));
                        ln[i]["a1"] = 1.0 / G - get_u(Tth[calib["instrument_temerature_sensor_backup"].get<bool>() ? 3 : 0], i) * ((calib_lines[l].calibration_views[i][0] + calib_lines[l].calibration_views[i][1]) / pow(G, 2.0));
                        ln[i]["a2"] = get_u(Tth[calib["instrument_temerature_sensor_backup"].get<bool>() ? 3 : 0], i) * (1.0 / pow(G, 2.0));
#endif
                    }
                }
                calib_out["vars"]["perLine_perChannel"].push_back(ln);
            }
            calib_out["wavenumbers"] = calib["wavenumber"];
        }
        // NOAA specific functions
        double MHSReader::get_timestamp(int pkt, int offset, int /*ms_scale*/)
        {
            if (pkt == 2)
            {
                uint32_t seconds_since_epoch = MIU_data[0][42] << 24 | MIU_data[0][43] << 16 | MIU_data[0][44] << 8 | MIU_data[0][45];
                uint16_t subsecond_time = MIU_data[0][46] << 8 | MIU_data[0][47];
                return offset * 86400.0 + double(seconds_since_epoch) + double(subsecond_time) * 15.3e-6 - SEC_OFFSET;
            }
            else if (pkt == 0)
            {
                uint32_t seconds_since_epoch = MIU_data[27][26] << 24 | MIU_data[27][27] << 16 | MIU_data[27][28] << 8 | MIU_data[27][29];
                uint16_t subsecond_time = MIU_data[27][30] << 8 | MIU_data[27][31];
                return offset * 86400.0 + double(seconds_since_epoch) + double(subsecond_time) * 15.3e-6 - SEC_OFFSET;
            }
            else
            {
                uint32_t seconds_since_epoch = MIU_data[54][8] << 24 | MIU_data[54][9] << 16 | MIU_data[54][10] << 8 | MIU_data[54][11];
                uint16_t subsecond_time = MIU_data[54][12] << 8 | MIU_data[54][13];
                return offset * 86400.0 + double(seconds_since_epoch) + double(subsecond_time) * 15.3e-6 - SEC_OFFSET;
            }
        }

        std::array<uint8_t, SCI_PACKET_SIZE> MHSReader::get_SCI_packet(int PKT)
        {
            std::array<uint8_t, SCI_PACKET_SIZE> out;
            std::memset(&out[0], 0, SCI_PACKET_SIZE);
            if (PKT == 0)
            {
                int c = 0;
                for (int i = 80; i <= 97; i++)
                {
                    out[c] = MIU_data[27][i - MIU_BYTE_OFFSET];
                    c++;
                }
                for (int word = 28; word <= 52; word++)
                {
                    for (int i = 48; i <= 97; i++)
                    {
                        out[c] = MIU_data[word][i - MIU_BYTE_OFFSET];
                        c++;
                    }
                }
                for (int i = 48; i <= 65; i++)
                {
                    out[c] = MIU_data[53][i - MIU_BYTE_OFFSET];
                    c++;
                }
            }
            else if (PKT == 1)
            {
                int c = 0;
                for (int i = 62; i <= 97; i++)
                {
                    out[c] = MIU_data[54][i - MIU_BYTE_OFFSET];
                    c++;
                }
                for (int word = 55; word <= 79; word++)
                {
                    for (int i = 48; i <= 97; i++)
                    {
                        out[c] = MIU_data[word][i - MIU_BYTE_OFFSET];
                        c++;
                    }
                }
            }
            else if (PKT == 2)
            {
                int c = 0;
                for (int i = 96; i <= 97; i++)
                {
                    out[c] = MIU_data[0][i - MIU_BYTE_OFFSET];
                    c++;
                }
                for (int word = 1; word <= 25; word++)
                {
                    for (int i = 48; i <= 97; i++)
                    {
                        out[c] = MIU_data[word][i - MIU_BYTE_OFFSET];
                        c++;
                    }
                }
                for (int i = 48; i <= 81; i++)
                {
                    out[c] = MIU_data[26][i - MIU_BYTE_OFFSET];
                    c++;
                }
            }
            // test_out.write((char *)&out[0], SCI_PACKET_SIZE);
            return out;
        }

        void MHSReader::work_NOAA(uint8_t *buffer)
        {
            uint8_t cycle = buffer[7];
            if (cycle % 20 == 0)
                major_cycle_count = buffer[98] << 24 | buffer[99] << 16 | buffer[100] << 8 | buffer[101];

            if (major_cycle_count < last_major_cycle)
                last_major_cycle = major_cycle_count; // little failsafe for corrupted data

            if (major_cycle_count > last_major_cycle)
            {
                last_major_cycle = major_cycle_count;

                // get each science packet from the MIU frame
                for (int p = 0; p < 3; p++)
                {
                    int pk = (p == 0 ? 2 : p - 1); // get packet index, since they are not in order

                    std::array<uint8_t, SCI_PACKET_SIZE> SCI_packet = get_SCI_packet(pk);
                    timestamps.push_back(get_timestamp(pk, DAY_OFFSET));
                    work(SCI_packet.data());
                }
                std::memset(MIU_data, 0, 80 * 50);
            }

            for (int i = 0; i < 50; i++)
            {
                if (cycle < 80)
                    MIU_data[cycle][i] = buffer[i + 48]; // reading MIU data from AIP
            }
        }
        // metop specific functions
        void MHSReader::work_metop(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1302)
                return;
            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            work(&packet.payload[14]);
        }

        // telemetry dump
        nlohmann::json MHSReader::dump_telemetry(nlohmann::json calib_coefs)
        {
            std::function AB = [](bool i)
            { return (i ? "B" : "A"); };
            std::string mode[16] = {"power on",
                                    "warm up",
                                    "stand by",
                                    "scan",
                                    "fixed view",
                                    "self test",
                                    "safeing",
                                    "fault",
                                    "INVALID",
                                    "INVALID",
                                    "INVALID",
                                    "INVALID",
                                    "INVALID",
                                    "INVALID",
                                    "INVALID",
                                    "memory data packet ID"};

            std::string temperature_id[24] = {"LO H1",
                                              "LO H2",
                                              "LO H3/H4",
                                              "LO H5",
                                              "Mixer/LNA/Multiplexer H1",
                                              "Mixer/LNA/Multiplexer H2",
                                              "Mixer/LNA/Multiplexer H3/4",
                                              "Mixer/LNA/Multiplexer H5",
                                              "Quasi-optics baseplate #1 (dichroic D1(A) or polarizer(B))",
                                              "Quasi-optics baseplate #2 (dichroic D2(A) or mirror(B))",
                                              "IF baseplate #1",
                                              "IF baseplate #2",
                                              "scan mechanism core",
                                              "scan mechanism housing",
                                              "RDM SSHM",
                                              "FDM SSHM",
                                              "Structure 1 (-A edge, next to baseplate cutout)",
                                              "Structure 2 (-A edge, in-between Rx and SM)",
                                              "Structure 3 (-V edge, in-between EE and SM)",
                                              "processor module",
                                              "Main DC/DC converter module",
                                              "SCE RDM module",
                                              "SCE FDM module",
                                              "RF DC/DC converter module"};

            std::string current_id[6] = {"EE and SM +5V ",
                                         "receiver +8V",
                                         "receiver +15V",
                                         "receiver -15V",
                                         "RDM motor",
                                         "FDM motor"};
            nlohmann::json telemetry;
            for (calib_line cl : calib_lines)
            {
                // word 0
                telemetry["misc"]["mode"].push_back(mode[(cl.HK[0] >> 4)]);
                telemetry["misc"]["PIE ID"].push_back(AB((cl.HK[0] >> 3) & 1));
                telemetry["misc"]["sub-commutation code"].push_back(cl.HK[0] & 3);
                // word 1-2
                telemetry["misc"]["TC"]["clean"].push_back((bool)(cl.HK[1] >> 7));
                telemetry["misc"]["TC"]["conforms to CCSDS"].push_back((bool)((cl.HK[1] >> 6) & 1));
                telemetry["misc"]["TC"]["ecognized as command"].push_back((bool)((cl.HK[1] >> 5) & 1));
                telemetry["misc"]["TC"]["legal"].push_back((bool)((cl.HK[1] >> 4) & 1));
                telemetry["misc"]["flags"]["FDM motor current trip"].push_back((bool)((cl.HK[1] >> 3) & 1));
                telemetry["misc"]["TC"]["APID"].push_back(((cl.HK[1] & 3) << 8) & cl.HK[2]);
                // word 3-4
                telemetry["misc"]["TC"]["sequence count"].push_back((cl.HK[3] << 6) & (cl.HK[4] >> 2));
                telemetry["misc"]["TC"]["received count"].push_back(cl.HK[4] & 3);
                // word 5
                telemetry["misc"]["flags"]["current monitor fault (PSU)"].push_back((bool)(cl.HK[5] >> 7));
                telemetry["misc"]["flags"]["thermistor monitor fault"].push_back((bool)((cl.HK[5] >> 6) & 1));
                telemetry["misc"]["flags"]["switch fault"].push_back((bool)((cl.HK[5] >> 5) & 1));
                telemetry["misc"]["flags"]["processor fault"].push_back((bool)((cl.HK[5] >> 4) & 1));
                telemetry["misc"]["flags"]["RDM motor current trip"].push_back((bool)((cl.HK[5] >> 3) & 1));
                telemetry["misc"]["flags"]["DC offset error"].push_back((bool)((cl.HK[5] >> 2) & 1));
                telemetry["misc"]["flags"]["scan control error"].push_back((bool)((cl.HK[5] >> 1) & 1));
                telemetry["misc"]["flags"]["reference clock error"].push_back((bool)(cl.HK[5] & 1));
                // word 6
                telemetry["switches"]["receiver channel H4 backend"].push_back((bool)(cl.HK[6] >> 7));
                telemetry["switches"]["receiver channel H3 backend"].push_back((bool)((cl.HK[6] >> 6) & 1));
                telemetry["switches"]["receiver channel  H3/H4 local oscillator selected"].push_back(AB((cl.HK[6] >> 5) & 1));
                telemetry["switches"]["receiver channel H3/H4 front-end"].push_back((bool)((cl.HK[6] >> 4) & 1));
                telemetry["switches"]["receiver channel H2 local oscillator selected"].push_back(AB((cl.HK[6] >> 3) & 1));
                telemetry["switches"]["receiver channel H2"].push_back((bool)((cl.HK[6] >> 2) & 1));
                telemetry["switches"]["receiver channel H1 local oscillator selected"].push_back(AB((cl.HK[6] >> 1) & 1));
                telemetry["switches"]["receiver channel H1"].push_back((bool)(cl.HK[6] & 1));
                // word 7
                telemetry["switches"]["PROM"].push_back((bool)(cl.HK[7] >> 7));
                telemetry["switches"]["signal processing electronics/scan control electronics"].push_back((bool)((cl.HK[7] >> 6) & 1));
                telemetry["switches"]["auxiliary operational heaters"].push_back((bool)((cl.HK[7] >> 5) & 1));
                telemetry["switches"]["scan mechanism operational heaters"].push_back((bool)((cl.HK[7] >> 4) & 1));
                telemetry["switches"]["receiver operational heaters"].push_back((bool)((cl.HK[7] >> 3) & 1));
                telemetry["switches"]["Rx CV"].push_back((bool)((cl.HK[7] >> 2) & 1));
                telemetry["switches"]["receiver channel H5 local oscillator selected"].push_back(AB((cl.HK[7] >> 1) & 1));
                telemetry["switches"]["receiver channel H5"].push_back((bool)(cl.HK[7] & 1));
                // word 8
                telemetry["switches"]["FDM motor current trip status enabled"].push_back(!(bool)(cl.HK[8] >> 7));
                telemetry["switches"]["RDM motor current trip status enabled"].push_back(!(bool)((cl.HK[8] >> 6) & 1));
                telemetry["switches"]["FDM motor supply"].push_back((bool)((cl.HK[8] >> 5) & 1));
                telemetry["switches"]["RDM motor supply"].push_back((bool)((cl.HK[8] >> 4) & 1));
                telemetry["switches"]["FDM motor sensors selected"].push_back(AB((cl.HK[8] >> 3) & 1));
                telemetry["switches"]["RDM motor sensors selected"].push_back(AB((cl.HK[8] >> 2) & 1));
                telemetry["switches"]["FDM zero position sensors"].push_back(AB((cl.HK[8] >> 1) & 1));
                telemetry["switches"]["RDM zero position sensors"].push_back(AB(cl.HK[8] & 1));
                // temperature
                for (int i = 0; i < 24; i++)
                {
                    double Tt = 0.0;
                    for (int j = 0; j <= 4; j++)
                    {
                        Tt += (calib_coefs["g"][j].get<double>() * pow((double)cl.HK[i + HKTH_offset], (double)j));
                    }
                    telemetry["temperatures"][temperature_id[i]].push_back(Tt);
                }
                // current
                for (int c = 0; c < 6; c++)
                {
                    telemetry["currents"][current_id[c]].push_back(calib_coefs["current_coefs"][c][0].get<double>() + calib_coefs["current_coefs"][c][1].get<double>() * cl.HK[c + 33]);
                }
            }
            return telemetry;
        }
    }
}