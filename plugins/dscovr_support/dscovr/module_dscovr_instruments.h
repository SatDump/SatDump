#pragma once

#include "core/module.h"

#include "instruments/epic/epic_reader.h"

namespace dscovr
{
    namespace instruments
    {
        class DSCOVRInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            epic::EPICReader epic_reader;

            // Statuses
            instrument_status_t epic_status = DECODING;

        public:
            DSCOVRInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
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