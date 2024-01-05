#pragma once

#include "core/module.h"
#include "instruments/suvi/suvi_reader.h"

namespace goes
{
    namespace instruments
    {
        class GOESRInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            suvi::SUVIReader suvi_reader;

            // Statuses
            // instrument_status_t suvi_status = DECODING;

        public:
            GOESRInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    }
}