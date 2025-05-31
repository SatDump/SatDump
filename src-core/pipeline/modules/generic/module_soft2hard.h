#pragma once

#include "common/widgets/constellation.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include <complex>
#include <fstream>
#include <thread>

namespace satdump
{
    namespace pipeline
    {
        namespace generic
        {
            class Soft2HardModule : public base::FileStreamToFileStreamModule
            {
            protected:
                int8_t *input_buffer;

            public:
                Soft2HardModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~Soft2HardModule();
                void process();
                void drawUI(bool window);

            public:
                static std::string getID();
                virtual std::string getIDM() { return getID(); }
                static nlohmann::json getParams() { return {}; }
                static std::shared_ptr<ProcessingModule> getInstance(nlohmann::json parameters, std::string input_file, std::string output_file_hint);
            };
        } // namespace generic
    } // namespace pipeline
} // namespace satdump