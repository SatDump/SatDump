#pragma once

/**
 * @file module_ggak.h
 * @brief GGAK space environment decoder pipeline module.
 *
 * Reads 224-byte CADUs from the GGAK-VE / GGAK-E telemetry stream,
 * decodes magnetometer, particle, and spectrometer data, and produces
 * a GGAKProduct for the SatDump explorer.
 */

#include "ggak_reader.h"
#include "ggak_product.h"
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
            GGAKReader reader;

            // Shared state for decomposed process() phases
            CounterGapInfo master_gap_info{0, 0, 0};
            std::map<std::string, int> per_inst_gaps;
            std::map<std::string, int> per_inst_missing;

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

        private:
            void decodeFrames();
            void refreshUiCounters();
            void logSummary();
            void buildProduct(GGAKProduct &product);

        public:
            GGAKDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file,
                                                                 std::string output_file_hint,
                                                                 nlohmann::json parameters);
        };
    } // namespace ggak
} // namespace elektro_arktika
