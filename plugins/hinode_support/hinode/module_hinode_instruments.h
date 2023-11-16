#pragma once

#include "core/module.h"
#include "instruments/common.h"

namespace hinode
{
    namespace instruments
    {
        class HinodeInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            HinodeDepacketizer depack_flt_obs1, depack_flt_obs2;
            HinodeDepacketizer depack_spp_obs1, depack_spp_obs2;
            HinodeDepacketizer depack_xrt_obs1, depack_xrt_obs2;
            HinodeDepacketizer depack_eis_obs1, depack_eis_obs2;

            // Statuses
            instrument_status_t general_status = DECODING;

        public:
            HinodeInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
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