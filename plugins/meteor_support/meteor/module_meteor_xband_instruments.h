#pragma once

#include "meteor_xband_types.h"
#include "pipeline/module.h"
#include "pipeline/modules/instrument_utils.h"

namespace meteor
{
    namespace instruments
    {
        using namespace satdump::pipeline; // TODOREWORK

        class MeteorXBandInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            dump_instrument_type_t d_instrument_mode;

            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            int mtvza_lines = 0;

            // Statuses
            // instrument_status_t msumr_status = DECODING;
            instrument_status_t mtvza_status = DECODING;

        public:
            MeteorXBandInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace meteor