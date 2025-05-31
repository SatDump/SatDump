#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

namespace goes
{
    namespace mdl
    {
        class GOESMDLDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            uint8_t *buffer;

            bool locked = false;
            int cor;

            // UI Stuff
            float cor_history[200];

        public:
            GOESMDLDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GOESMDLDecoderModule();
            void process();
            void drawUI(bool window);
            nlohmann::json getModuleStats();

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace mdl
} // namespace goes