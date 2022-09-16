#pragma once

#include "core/module.h"
#include "instruments/chris/chris_reader.h"
#include "instruments/hrc/hrc_reader.h"
#include "instruments/swap/swap_reader.h"

namespace proba
{
    namespace instruments
    {
        class PROBAInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            enum proba_sat_t
            {
                PROBA_1,
                PROBA_2,
                PROBA_V,
            };

            proba_sat_t d_satellite;

            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

            // Readers
            std::unique_ptr<chris::CHRISReader> chris_reader;
            std::unique_ptr<hrc::HRCReader> hrc_reader;
            std::unique_ptr<swap::SWAPReader> swap_reader;

            // Statuses
            instrument_status_t chris_status = DECODING;
            instrument_status_t hrc_status = DECODING;
            instrument_status_t swap_status = DECODING;

        public:
            PROBAInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace amsu
} // namespace metop