#pragma once

#include "instruments/common.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace hinode
{
    namespace instruments
    {
        class HinodeInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
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
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace hinode