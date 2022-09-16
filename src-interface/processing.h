#pragma once

#include <mutex>
#include <memory>
#include "core/module.h"

namespace satdump
{
    namespace processing
    {
        extern std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> ui_call_list;
        extern std::shared_ptr<std::mutex> ui_call_list_mutex;
        extern bool is_processing;

        void process(std::string downlink_pipeline,
                     std::string input_level,
                     std::string input_file,
                     std::string output_file,
                     nlohmann::json parameters);
    }
}