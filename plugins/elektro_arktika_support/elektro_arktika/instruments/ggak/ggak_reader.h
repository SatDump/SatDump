#pragma once

/**
 * @file ggak_reader.h
 * @brief GGAK frame reader/decoder.
 *
 * Accumulates decoded readings from GGAK-VE / GGAK-E 224-byte CADUs.
 * Follows the SatDump reader pattern (cf. MSUVISReader, MSUIRReader).
 */

#include "ggak_types.h"

#include <map>
#include <vector>

namespace elektro_arktika
{
    namespace ggak
    {
        class GGAKReader
        {
        public:
            void pushFrame(const uint8_t *cadu);

            // Decoded readings
            std::vector<MagReading> mag_readings;
            std::vector<ParticleReading> particle_readings;
            std::vector<SubPacket> subpackets_20; // SKIF-VE/V ESA
            std::vector<SubPacket> subpackets_30; // SKIF-VE/G ESA
            std::vector<SubPacket> subpackets_5c; // SKL-E spectral
            std::vector<SubPacket> subpackets_00; // Housekeeping-A
            std::vector<SubPacket> subpackets_10; // SKIF-VE SER summary

            // Frame accounting
            int total_frames = 0;
            int fill_frames = 0;
            int checksum_pass = 0;
            int checksum_fail = 0;
            std::map<uint8_t, int> frame_counts;
            std::vector<uint16_t> master_counters;
            std::map<uint8_t, std::vector<uint16_t>> channel_counters;

            // Detected satellite profile
            SatelliteProfile sat_profile = PROFILE_BND_VE;
            bool profile_detected = false;
            uint8_t observed_source_id = 0;

            // Sort all accumulated readings by elapsed time
            void sortAll();

        private:
            int prev_coarse_time = -1;
            uint32_t coarse_time_offset = 0;
            std::vector<uint8_t> spectral_buffer;

            static void decode_magnetometer_frame(const uint8_t *payload, size_t len,
                                                  uint32_t ct, uint8_t ft, uint8_t data_fill,
                                                  bool csum_ok, std::vector<MagReading> &out);

            static void decode_particle_frame(const uint8_t *payload, size_t len,
                                              uint32_t ct, uint8_t ft, uint8_t data_fill,
                                              bool csum_ok, std::vector<ParticleReading> &out);

            static void decode_subpacket_69(const uint8_t *payload, size_t len,
                                            uint8_t marker_hi,
                                            uint32_t ct, uint8_t ft, uint8_t data_fill,
                                            bool csum_ok, std::vector<SubPacket> &out);

            static void decode_housekeeping_12(const uint8_t *payload, size_t len,
                                               uint8_t marker_byte,
                                               uint32_t ct, uint8_t ft, uint8_t data_fill,
                                               bool csum_ok, std::vector<SubPacket> &out);

            static void decode_spectral(const uint8_t *payload, size_t len,
                                        uint8_t marker,
                                        uint32_t ct, uint8_t ft, uint8_t data_fill,
                                        bool csum_ok, std::vector<SubPacket> &out,
                                        std::vector<uint8_t> &buffer);
        };
    } // namespace ggak
} // namespace elektro_arktika
