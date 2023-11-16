#pragma once

#include "core/module.h"

#include "instruments/modis/modis_reader.h"
#include "../aqua/instruments/airs/airs_reader.h"
#include "../aqua/instruments/amsu/amsu_a1_reader.h"
#include "../aqua/instruments/amsu/amsu_a2_reader.h"
#include "../aqua/instruments/ceres/ceres_reader.h"
#include "../aura/instruments/omi/omi_reader.h"

#include "../aqua/instruments/gbad/gbad_reader.h"

namespace eos
{
    namespace instruments
    {
        class EOSInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            enum eos_sat_t
            {
                TERRA,
                AQUA,
                AURA,
            };

            eos_sat_t d_satellite;
            bool d_modis_bowtie;

            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            modis::MODISReader modis_reader;
            aqua::airs::AIRSReader airs_reader;
            aqua::amsu::AMSUA1Reader amsu_a1_reader;
            aqua::amsu::AMSUA2Reader amsu_a2_reader;
            aqua::ceres::CERESReader ceres_fm3_reader;
            aqua::ceres::CERESReader ceres_fm4_reader;
            aura::omi::OMIReader omi_1_reader;
            aura::omi::OMIReader omi_2_reader;

            aqua::gbad::GBADReader gbad_reader;

            // Statuses
            instrument_status_t modis_status = DECODING;
            instrument_status_t airs_status = DECODING;
            instrument_status_t amsu_status = DECODING;
            instrument_status_t ceres_fm3_status = DECODING;
            instrument_status_t ceres_fm4_status = DECODING;
            instrument_status_t omi_status = DECODING;

        public:
            EOSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace modis
} // namespace eos