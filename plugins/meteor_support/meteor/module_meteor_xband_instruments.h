#pragma once

#include "core/module.h"
#include "meteor_xband_types.h"

namespace meteor
{
    namespace instruments
    {
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
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace amsu
} // namespace Meteor