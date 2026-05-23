#pragma once

#include "ggak_types.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

#include <atomic>
#include <map>

namespace elektro_arktika
{
    namespace ggak
    {
        class GGAKDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            // Per-instrument accumulated readings
            std::vector<MagReading> mag_readings;
            std::vector<ParticleReading> particle_readings;
            std::vector<SubPacket> subpackets_20; // SKIF-VE/V ESA
            std::vector<SubPacket> subpackets_30; // SKIF-VE/G ESA
            std::vector<SubPacket> subpackets_5c; // SKL-E spectral
            std::vector<SubPacket> subpackets_00; // Housekeeping-A
            std::vector<SubPacket> subpackets_10; // SKIF-VE SER summary

            // Frame accounting
            std::map<uint8_t, int> frame_counts;
            std::vector<uint16_t> master_counters;
            std::map<uint8_t, std::vector<uint16_t>> channel_counters;
            int total_frames = 0;
            int fill_frames = 0;
            int checksum_pass = 0;
            int checksum_fail = 0;

            // Detected satellite profile
            SatelliteProfile sat_profile = PROFILE_BND_VE;
            bool profile_detected = false;

            // Instrument processing status (for drawUI)
            enum
            {
                STATUS_MAG = 0,
                STATUS_PARTICLES,
                STATUS_SKIF_ESA,
                STATUS_SKL,
                STATUS_HK,
                STATUS_BND,
                STATUS_COUNT
            };
            instrument_status_t inst_status[STATUS_COUNT] = {
                DECODING, DECODING, DECODING, DECODING, DECODING, DECODING};

            std::atomic<int> ui_total_frames{0};
            std::atomic<int> ui_fill_frames{0};
            std::atomic<int> ui_mag_count{0};
            std::atomic<int> ui_particle_count{0};
            std::atomic<int> ui_subpackets_20_count{0};
            std::atomic<int> ui_subpackets_30_count{0};
            std::atomic<int> ui_subpackets_5c_count{0};
            std::atomic<int> ui_subpackets_00_count{0};
            std::atomic<int> ui_subpackets_10_count{0};

        public:
            GGAKDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; }
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file,
                                                                 std::string output_file_hint,
                                                                 nlohmann::json parameters);
        };
    } // namespace ggak
} // namespace elektro_arktika
