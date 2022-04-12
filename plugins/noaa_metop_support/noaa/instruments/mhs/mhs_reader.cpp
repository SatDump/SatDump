/*

NOAA MHS decoder by ZbychuButItWasTaken

This decoder takes in raw AIP data and processes it to MHS. It perfprms calibration (based on NOAA KLM User's Guide) and rain detection (though not very well as N19's MHS is broken)

*/

#include "mhs_reader.h"
#include <cstring>

namespace noaa
{
    namespace mhs
    {
        MHSReader::MHSReader()
        {
            std::memset(MIU_data, 0, 80 * 50);
        }

        void MHSReader::work(uint8_t *buffer)
        {
            uint8_t cycle = buffer[7];
            if (cycle % 20 == 0)
                major_cycle_count = buffer[98] << 24 | buffer[99] << 16 | buffer[100] << 8 | buffer[101];

            if (major_cycle_count < last_major_cycle)
                last_major_cycle = major_cycle_count; // little failsafe for corrupted data

            if (major_cycle_count > last_major_cycle)
            {
                std::array<std::array<uint16_t, MHS_WIDTH>, 5> linebuff;
                std::array<std::array<uint16_t, 8>, 5> calibbuff;
                last_major_cycle = major_cycle_count;

                // get each science packet from the MIU frame
                for (int p = 0; p < 3; p++)
                {
                    int pk = (p == 0 ? 2 : p - 1); // get packet index, since they are not in order

                    std::array<uint8_t, SCI_PACKET_SIZE> SCI_packet = get_SCI_packet(pk); // get packet
                    timestamps.push_back(get_timestamp(pk, DAY_OFFSET));                  // push timestamp

                    std::memset(&linebuff, 0, MHS_WIDTH * 5 * 2); // make some room
                    std::memset(&calibbuff, 0, 8 * 5 * 2);

                    for (int i = 0; i < MHS_WIDTH; i++)
                    {
                        for (int c = 1; c < 6; c++)
                        {
                            linebuff[c - 1][i] = SCI_packet[MHS_OFFSET + i * 12 + c * 2] << 8 | SCI_packet[MHS_OFFSET + i * 12 + c * 2 + 1]; // extract image data
                        }
                    }
                    for (int c = 0; c < 5; c++)
                        channels[c].push_back(linebuff[c]);
                    line++;

                    //***************************
                    //*******CALIBRATION*********
                    //***************************

                    // declare all the variables
                    double a, b;
                    double R[5], Tk[5], WTk = 0, Wk = 0, Tw, Tavg = 0;
                    std::array<double, 24> Tth;

                    // read the needed values for calibration from the SCI Packet
                    std::array<uint16_t, 3> PRT_calib;
                    for (int i = 0; i < 3; i++)
                        PRT_calib[i] = (SCI_packet[PRT_OFFSET + i * 2 + 10] << 8) | SCI_packet[PRT_OFFSET + i * 2 + 11];

                    std::array<uint16_t, 5> PRT_readings;
                    for (int i = 0; i < 5; i++)
                        PRT_readings[i] = SCI_packet[PRT_OFFSET + i * 2] << 8 | SCI_packet[PRT_OFFSET + i * 2 + 1];

                    std::array<uint8_t, 24> HKTH;
                    for (int i = 0; i < 24; i++)
                        HKTH[i] = SCI_packet[i + HKTH_offset];

                    std::array<std::array<uint16_t, 2>, 5> calibration_views;
                    //get the calibration views from the blackbody and space
                    for (int c = 0; c < 5; c++)
                        for (int j = 0; j < 2; j++)
                            calibration_views[c][j] = 0;
                    for (int c = 0; c < 5; c++)
                    {
                        for (int j = 0; j < 2; j++)
                        {
                            for (int k = 0; k < 4; k++)
                                calibration_views[c][j] += ((SCI_packet[MHS_OFFSET + (j * 4 + k + MHS_WIDTH) * 12 + (c + 1) * 2] << 8 | SCI_packet[MHS_OFFSET + (j * 4 + k + MHS_WIDTH) * 12 + (c + 1) * 2 + 1]) / 4);
                        }
                    }

                    // math for calculating PRT temperature
                    double CCALn = PRT_calib[0] + PRT_calib[1] + PRT_calib[2];
                    double C2CALn = pow((double)PRT_calib[0], 2.0) + pow((double)PRT_calib[1], 2.0) + pow((double)PRT_calib[2], 2.0);
                    double RCCALn = (double)PRT_calib[0] * calibration::RCAL[0] + (double)PRT_calib[1] * calibration::RCAL[1] + (double)PRT_calib[2] * calibration::RCAL[2];
                    a = (RCALn * C2CALn - CCALn * RCCALn) / (3 * C2CALn - pow(CCALn, 2.0));
                    b = (3 * RCCALn - RCALn * CCALn) / (3 * C2CALn - pow(CCALn, 2.0));

                    for (int i = 0; i < 5; i++)
                    {
                        R[i] = a + b * PRT_readings[i];
                        Tk[i] = 0;
                        for (int j = 0; j <= 3; j++)
                        {
                            Tk[i] += calibration::f[i][j] * pow(R[i], (double)j);
                        }
                        WTk += (calibration::W[i] * Tk[i]);
                        Wk += calibration::W[i];
                    }
                    Tw = WTk / Wk;

                    // average instrument temperature
                    for (int i = 0; i < 24; i++)
                    {
                        Tth[i] = 0.0;
                        for (int j = 0; j <= 4; j++)
                        {
                            Tth[i] += (calibration::g[j] * pow((double)HKTH[i], (double)j));
                        }
                    }
                    for (int i = 1; i < 20; i++)
                    {
                        Tavg += Tth[i];
                    }
                    Tavg /= 19;

                    // calibration of each channel
                    for (int i = 0; i < 5; i++)
                    {
                        double Rw = temperature_to_radiance(Tw, calibration::wavenumber[i]);
                        double Rc = temperature_to_radiance(2.73, calibration::wavenumber[i]);

                        double CCw = calibration_views[i][1]; // blackbody count
                        double CCc = calibration_views[i][0]; // space count

                        double G = (CCw - CCc) / (Rw - Rc);

                        double a0 = Rw - (CCw / G) + get_u(Tavg, i) * ((CCw * CCc) / pow(G, 2.0));
                        double a1 = 1.0 / G - get_u(Tavg, i) * ((CCc + CCw) / pow(G, 2.0));
                        double a2 = get_u(Tavg, i) * (1.0 / pow(G, 2.0));
                    }

                    std::memset(MIU_data, 0, 80 * 50);
                }

                for (int i = 0; i < 50; i++)
                {
                    if (cycle < 80)
                        MIU_data[cycle][i] = buffer[i + 48]; // reading MIU data from AIP
                }
            }
        }

        image::Image<uint16_t> MHSReader::getChannel(int channel)
        {
            image::Image<uint16_t> output(MHS_WIDTH, line, 1);
            std::memset(&output[0], 0, MHS_WIDTH * line * 2);
            for (int l = 0; l < line; l++)
            {
                for (int x = 0; x < MHS_WIDTH; x++)
                {
                    output[l * MHS_WIDTH + (MHS_WIDTH - x - 1)] = channels[channel][l][x];
                }
            }
            return output;
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

        double MHSReader::get_u(double temp, int ch)
        {
            if (temp == calibration::u_temps[0])
                return calibration::u[0][ch];

            if (temp == calibration::u_temps[1])
                return calibration::u[1][ch];

            if (temp == calibration::u_temps[2])
                return calibration::u[2][ch];

            if (temp < calibration::u_temps[0])
                return interpolate(calibration::u_temps[0], calibration::u[0][ch], calibration::u_temps[1], calibration::u[1][ch], temp, 0);

            if (temp > calibration::u_temps[0] && temp < calibration::u_temps[1])
                return interpolate(calibration::u_temps[0], calibration::u[0][ch], calibration::u_temps[1], calibration::u[1][ch], temp, 0);

            if (temp > calibration::u_temps[1] && temp < calibration::u_temps[2])
                return interpolate(calibration::u_temps[1], calibration::u[1][ch], calibration::u_temps[2], calibration::u[2][ch], temp, 0);

            else
                return interpolate(calibration::u_temps[1], calibration::u[1][ch], calibration::u_temps[2], calibration::u[2][ch], temp, 1);
        }

        double MHSReader::interpolate(double a1x, double a1y, double a2x, double a2y, double bx, int mode)
        {                  // where a1y > a2y and a1x < a1y
            if (mode == 0) // between a1 and a2 or bx < a1x
                return ((a2x - bx) * (a1y - a2y)) / (a2x - a1x) + a2y;
            else // > a2x
                return -1 * (((bx - a2x) * (a1y - a2y)) / (a2x - a1x) - a2y);
        }

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
    }
}