#pragma once

#include "instruments/chris/chris_reader.h"
#include "instruments/hrc/hrc_reader.h"
#include "instruments/swap/swap_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

#include "instruments/vegetation/vegs_reader.h"

#include "instruments/gps_ascii/gps_ascii.h"
#include "pipeline/modules/instrument_utils.h"

namespace proba
{
    namespace instruments
    {
        class PROBAInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            enum proba_sat_t
            {
                PROBA_1,
                PROBA_2,
                PROBA_V,
            };

            proba_sat_t d_satellite;

            // Readers
            std::unique_ptr<chris::CHRISReader> chris_reader;
            std::unique_ptr<hrc::HRCReader> hrc_reader;
            std::unique_ptr<swap::SWAPReader> swap_reader;
            std::unique_ptr<vegetation::VegetationS> vegs_readers[3][6];
            std::unique_ptr<gps_ascii::GPSASCII> gps_ascii_reader;

            // Statuses
            instrument_status_t chris_status = DECODING;
            instrument_status_t hrc_status = DECODING;
            instrument_status_t swap_status = DECODING;
            instrument_status_t vegs_status[3][6];

        public:
            PROBAInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace proba