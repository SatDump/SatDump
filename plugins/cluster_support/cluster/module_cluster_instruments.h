#pragma once

#include "core/module.h"

#include "instruments/wbd_decoder.h"

namespace cluster
{
    namespace instruments
    {
        class CLUSTERInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            bool enable_audio = false;
            bool play_audio;

            // Readers
            // cips::CIPSReader cips_readers[4];
            // totally not stolen AIM decoder.....
            // Statuses
            // instrument_status_t cips_status[4] = {DECODING, DECODING, DECODING, DECODING};

            bool param_VCXO = false;
            bool param_OBDH = false;
            bool param_commands = false;
            int param_antenna = 0;
            int param_convfrq = 0;
            int param_band = 0;

        public:
            CLUSTERInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
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