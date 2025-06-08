#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

namespace angels
{
    namespace argos
    {
        class AngelsArgosDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        public:
            AngelsArgosDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace argos
} // namespace angels