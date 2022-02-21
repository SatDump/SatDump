#pragma once

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include "mhs_calibration_values.h"
#include "common/image/image.h"

#define SCI_PACKET_SIZE 1286
#define MIU_BYTE_OFFSET 48
#define MHS_OFFSET 49
#define MHS_WIDTH 90
#define PRT_OFFSET 1225
#define HKTH_offset 8

#define DAY_OFFSET 17453
#define SEC_OFFSET 3600 * 9 - 60 * 10

#define c1 1.191042e-05
#define c2 1.4387752
#define e_num 2.7182818

namespace noaa
{
    namespace mhs
    {
        class MHSReader
        {
        private:
            std::array<std::vector<std::array<uint16_t, MHS_WIDTH>>, 5> channels;
            std::array<std::vector<std::array<double, MHS_WIDTH>>, 5> calibrated_channels;
            unsigned int last = 0;
            std::array<uint8_t, 50> MIU_data[80];
            uint32_t major_cycle_count = 0;
            uint32_t last_major_cycle = 0;

            //things needed for calibration
            std::vector<std::array<std::array<uint16_t, 2>, 5>> calibration;
            std::vector<std::array<uint16_t, 5>> PRT_readings;
            std::vector<std::array<uint16_t, 3>> PRT_calib;
            std::vector<std::array<uint8_t, 24>> HKTH;
            std::array<uint8_t, SCI_PACKET_SIZE> get_SCI_packet(int PKT);
            std::array<std::array<uint16_t, 2>, 5> get_calibration_data(std::array<uint8_t, SCI_PACKET_SIZE> &packet);
            std::array<uint16_t, 5> get_PRTs(std::array<uint8_t, SCI_PACKET_SIZE> &packet);
            std::array<uint16_t, 3> get_PRT_calib(std::array<uint8_t, SCI_PACKET_SIZE> &packet);
            std::array<uint8_t, 24> get_HKTH(std::array<uint8_t, SCI_PACKET_SIZE> &packet);
            double temp_to_rad(double t, double v);
            double rad_to_temp(double r, double v);
            double get_avg_count(int line, int ch, int blackbody);
            double get_u(double temp, int ch);
            double interpolate(double a1x, double a1y, double a2x, double a2y, double bx, int mode);
            double get_timestamp(int pkt, int offset, int ms_scale = 1000);
            std::array<std::array<uint16_t, 2>, 5> tmp;

        public:
            MHSReader();
            int line = 0;
            std::vector<double> timestamps;
            void work(uint8_t *buffer);
            image::Image<uint16_t> getChannel(int channel);
            std::vector<double> get_calibrated_channel(int channel);
            void calibrate();
        };
    } // namespace hirs
} // namespace noaa