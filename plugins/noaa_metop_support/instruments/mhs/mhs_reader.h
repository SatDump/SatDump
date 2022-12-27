#pragma once

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include "mhs_calibration.h"
#include "common/image/image.h"
#include "common/calibration.h"

#include "common/ccsds/ccsds.h"
#include "common/ccsds/ccsds_time.h"

#define SCI_PACKET_SIZE 1286
#define MIU_BYTE_OFFSET 48
#define MHS_OFFSET 49
#define MHS_WIDTH 90
#define PRT_OFFSET 1225
#define HKTH_offset 8

#define DAY_OFFSET 17453
#define SEC_OFFSET 3600 * 9 - 60 * 10

namespace noaa_metop
{
    namespace mhs
    {
        class MHSReader
        {
        private:
            std::array<std::vector<std::array<uint16_t, MHS_WIDTH>>, 5> channels;
            std::array<std::vector<std::array<double, MHS_WIDTH>>, 5> calibrated_channels;
            void work(uint8_t *buffer, int id);

            //NOAA specific stuff
            unsigned int last = 0;
            std::array<uint8_t, 50> MIU_data[80];
            uint32_t major_cycle_count = 0;
            uint32_t last_major_cycle = 0;

            //calib values
            MHS_calibration_Values calib;

            //calib functions
            std::array<uint8_t, SCI_PACKET_SIZE> get_SCI_packet(int PKT);
            double get_u(double temp, int ch);
            double interpolate(double a1x, double a1y, double a2x, double a2y, double bx, int mode);
            double get_timestamp(int pkt, int offset, int ms_scale = 1000);
            MHS_calibration_Values get_calibration_values(int id);


        public:
            MHSReader();
            int line = 0;
            std::vector<double> timestamps;
            void work_NOAA(uint8_t *buffer);
            void work_metop(ccsds::CCSDSPacket &packet, int id);
            image::Image<uint16_t> getChannel(int channel);
            std::array<std::vector<std::vector<double>>, 5> calibration_coefs;
        };
    } // namespace hirs
} // namespace noaa