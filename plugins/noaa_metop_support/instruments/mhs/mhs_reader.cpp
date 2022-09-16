/*

NOAA MHS decoder by ZbychuButItWasTaken

This decoder takes in raw AIP data and processes it to MHS. It perfprms calibration (based on NOAA KLM User's Guide) and rain detection (though not very well as N19's MHS is broken)

*/

#include "mhs_reader.h"
#include <cstring>

#include "logger.h"

namespace noaa_metop
{
    namespace mhs
    {
        MHSReader::MHSReader()
        {
            std::memset(MIU_data, 0, 80 * 50);
        }

        void MHSReader::work(uint8_t *buffer, int id)
        {
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

            //***************************
            //*******CALIBRATION*********
            //***************************

            calib = get_calibration_values(id);
            double RCALn = calib.RCAL[0] + calib.RCAL[1] + calib.RCAL[2];

            // declare all the variables
            double a, b;
            double R[5], Tk[5], WTk = 0, Wk = 0, Tw, Tavg = 0;
            std::array<double, 24> Tth;

            // read the needed values for calibration from the SCI Packet
            std::array<uint16_t, 3> PRT_calib;
            for (int i = 0; i < 3; i++)
                PRT_calib[i] = (buffer[PRT_OFFSET + i * 2 + 10] << 8) | buffer[PRT_OFFSET + i * 2 + 11];

            std::array<uint16_t, 5> PRT_readings;
            for (int i = 0; i < 5; i++)
                PRT_readings[i] = buffer[PRT_OFFSET + i * 2] << 8 | buffer[PRT_OFFSET + i * 2 + 1];

            std::array<uint8_t, 24> HKTH;
            for (int i = 0; i < 24; i++)
                HKTH[i] = buffer[i + HKTH_offset];

            std::array<std::array<uint16_t, 2>, 5> calibration_views;
            // get the calibration views from the blackbody and space
            for (int c = 0; c < 5; c++)
                for (int j = 0; j < 2; j++)
                    calibration_views[c][j] = 0;
            for (int c = 0; c < 5; c++)
            {
                for (int j = 0; j < 2; j++)
                {
                    for (int k = 0; k < 4; k++)
                        calibration_views[c][j] += ((buffer[MHS_OFFSET + (j * 4 + k + MHS_WIDTH) * 12 + (c + 1) * 2] << 8 | buffer[MHS_OFFSET + (j * 4 + k + MHS_WIDTH) * 12 + (c + 1) * 2 + 1]) / 4);
                }
            }

            // math for calculating PRT temperature
            double CCALn = PRT_calib[0] + PRT_calib[1] + PRT_calib[2];
            double C2CALn = pow((double)PRT_calib[0], 2.0) + pow((double)PRT_calib[1], 2.0) + pow((double)PRT_calib[2], 2.0);
            double RCCALn = (double)PRT_calib[0] * calib.RCAL[0] + (double)PRT_calib[1] * calib.RCAL[1] + (double)PRT_calib[2] * calib.RCAL[2];
            a = (RCALn * C2CALn - CCALn * RCCALn) / (3 * C2CALn - pow(CCALn, 2.0));
            b = (3 * RCCALn - RCALn * CCALn) / (3 * C2CALn - pow(CCALn, 2.0));

            for (int i = 0; i < 5; i++)
            {
                R[i] = a + b * PRT_readings[i];
                Tk[i] = 0;
                for (int j = 0; j <= 3; j++)
                {
                    Tk[i] += calib.f[i][j] * pow(R[i], (double)j);
                }
                WTk += (calib.W[i] * Tk[i]);
                Wk += calib.W[i];
            }
            Tw = WTk / Wk;

            // average instrument temperature
            for (int i = 0; i < 24; i++)
            {
                Tth[i] = 0.0;
                for (int j = 0; j <= 4; j++)
                {
                    Tth[i] += (calib.g[j] * pow((double)HKTH[i], (double)j));
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
                double Rw = temperature_to_radiance(Tw, calib.wavenumber[i]);
                double Rc = temperature_to_radiance(2.73, calib.wavenumber[i]);

                double CCw = calibration_views[i][1]; // blackbody count
                double CCc = calibration_views[i][0]; // space count

                double G = (CCw - CCc) / (Rw - Rc);

                double a0 = Rw - (CCw / G) + get_u(Tavg, i) * ((CCw * CCc) / pow(G, 2.0));
                double a1 = 1.0 / G - get_u(Tavg, i) * ((CCc + CCw) / pow(G, 2.0));
                double a2 = get_u(Tavg, i) * (1.0 / pow(G, 2.0));

                // logger->info(std::to_string(a0) + " " + std::to_string(a1) + " " + std::to_string(a2));
                calibration_coefs[i].push_back({a0, a1, a2});
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

        double MHSReader::get_u(double temp, int ch)
        {
            if (temp == calib.u_temps[0])
                return calib.u[0][ch];

            if (temp == calib.u_temps[1])
                return calib.u[1][ch];

            if (temp == calib.u_temps[2])
                return calib.u[2][ch];

            if (temp < calib.u_temps[0])
                return interpolate(calib.u_temps[0], calib.u[0][ch], calib.u_temps[1], calib.u[1][ch], temp, 0);

            if (temp > calib.u_temps[0] && temp < calib.u_temps[1])
                return interpolate(calib.u_temps[0], calib.u[0][ch], calib.u_temps[1], calib.u[1][ch], temp, 0);

            if (temp > calib.u_temps[1] && temp < calib.u_temps[2])
                return interpolate(calib.u_temps[1], calib.u[1][ch], calib.u_temps[2], calib.u[2][ch], temp, 0);

            else
                return interpolate(calib.u_temps[1], calib.u[1][ch], calib.u_temps[2], calib.u[2][ch], temp, 1);
        }

        double MHSReader::interpolate(double a1x, double a1y, double a2x, double a2y, double bx, int mode)
        {                  // where a1y > a2y and a1x < a1y
            if (mode == 0) // between a1 and a2 or bx < a1x
                return ((a2x - bx) * (a1y - a2y)) / (a2x - a1x) + a2y;
            else // > a2x
                return -1 * (((bx - a2x) * (a1y - a2y)) / (a2x - a1x) - a2y);
        }

        MHS_calibration_Values MHSReader::get_calibration_values(int id)
        {
            if (id == 0)
                return {
                    {{26.47922, 2.381473, 7.897082E-04, 6.277443E-07},
                     {25.72883, 2.401215, 6.160758E-04, 1.114127E-06},
                     {26.75906, 2.375112, 8.306877E-04, 5.707352E-07},
                     {25.80272, 2.401580, 5.882624E-04, 1.307091E-06},
                     {26.78935, 2.370714, 9.064551E-04, 2.251251E-05}},
                    {117.988, 95.289, 80.602},
                    {355.9982, -0.239278, -4.85712E-03, 3.59838E-05, -8.02652E-08},
                    {275.28, 288.00, 299.45},
                    {{1.709693E-02, 5.452098E-02, 3.468686E-02, 2.263756E-02, 7.784018E-03},
                     {1.068579E-01, 8.246891E-02, 5.056471E-02, 2.977869E-02, 2.917676E-02},
                     {1.914666E-01, 1.071100E-01, 4.829732E-02, 4.355207E-02, 3.218666E-02}},
                    {-0.0031, 1.00027},
                    {2.968720, 5.236956, 6.114597, 6.114597, 6.348092},
                    {1, 1, 1, 1, 2},
                    {-1.5, 10, 39, 48, 20},
                    {0.1, 66, 205, 70, 67}};
            else if (id == 1)
                return MHS_calibration_Values(); /// Metop A
            else if (id == 2)
                return MHS_calibration_Values(); // metop B
            else if (id == 3)
                return MHS_calibration_Values(); // metop C
            else
                return MHS_calibration_Values(); // Shouldn't ever happen
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
                    work(SCI_packet.data(), 0);
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

        void MHSReader::work_metop(ccsds::CCSDSPacket &packet, int id)
        {
            if (packet.payload.size() < 1302)
                return;
            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            work(&packet.payload[14], id);
        }
    }
}