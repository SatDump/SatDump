#pragma once

#include "core/module.h"

#include "instruments/amsr2/amsr2_reader.h"

namespace gcom1
{
    namespace instruments
    {
        class GCOM1InstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

            // Readers
            amsr2::AMSR2Reader amsr2_reader;

            // Statuses
            instrument_status_t amsr2_status = DECODING;

        public:
            GCOM1InstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
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