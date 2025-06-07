#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

namespace satdump
{
    namespace pipeline
    {
        namespace xrit
        {
            class S2UDPxRITCADUextractor : public base::FileStreamToFileStreamModule
            {
            protected:
                int bbframe_size;
                int pid_to_decode;

                // UI Stuff
                int current_pid;

            public:
                S2UDPxRITCADUextractor(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~S2UDPxRITCADUextractor();
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