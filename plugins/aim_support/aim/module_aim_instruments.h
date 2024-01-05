#pragma once

#include "core/module.h"

#include "instruments/cips/cips_reader.h"

namespace aim
{
    namespace instruments
    {
        class AIMInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            cips::CIPSReader cips_readers[4];

            // Statuses
            instrument_status_t cips_status[4] = {DECODING, DECODING, DECODING, DECODING};

        public:
            AIMInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
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