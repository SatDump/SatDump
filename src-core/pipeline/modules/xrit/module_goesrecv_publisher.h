#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

namespace satdump
{
    namespace pipeline
    {
        namespace xrit
        {
            class GOESRecvPublisherModule : public base::FileStreamToFileStreamModule
            {
            protected:
                uint8_t *buffer;

                std::string address;
                int port;

                // UI Stuff
                float ber_history[200];
                float cor_history[200];

            public:
                GOESRecvPublisherModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~GOESRecvPublisherModule();
                void process();
                void drawUI(bool window);
                std::vector<ModuleDataType> getInputTypes();
                std::vector<ModuleDataType> getOutputTypes();

            public:
                static std::string getID();
                virtual std::string getIDM() { return getID(); };
                static nlohmann::json getParams() { return {}; } // TODOREWORK
                static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            };
        } // namespace xrit
    } // namespace pipeline
} // namespace satdump