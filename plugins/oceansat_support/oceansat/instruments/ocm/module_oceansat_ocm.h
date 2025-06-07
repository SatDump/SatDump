#pragma once

#include "ocm_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace oceansat
{
    namespace ocm
    {
        class OceansatOCMDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            OCMReader ocm_reader;
            instrument_status_t ocm_status = DECODING;

        public:
            OceansatOCMDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace ocm
} // namespace oceansat