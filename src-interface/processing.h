#pragma once

#include <mutex>
#include <memory>
#include "pipeline/pipeline.h"
#include "dll_export.h"

namespace satdump
{
    namespace processing
    {
        SATDUMP_DLL2 extern std::shared_ptr<std::vector<std::shared_ptr<pipeline::ProcessingModule>>> ui_call_list;
        SATDUMP_DLL2 extern std::shared_ptr<std::mutex> ui_call_list_mutex;
        SATDUMP_DLL2 extern bool is_processing;

        void process(pipeline::Pipeline downlink_pipeline,
                     std::string input_level,
                     std::string input_file,
                     std::string output_file,
                     nlohmann::json parameters);

        void process(std::string downlink_pipeline,
                     std::string input_level,
                     std::string input_file,
                     std::string output_file,
                     nlohmann::json parameters);
    }
}