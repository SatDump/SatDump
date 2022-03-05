#pragma once

#include <mutex>
#include <memory>
#include "core/module.h"
#include "pipeline.h"

extern std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList;
extern std::shared_ptr<std::mutex> uiCallListMutex;

namespace processing
{
    void process(std::string downlink_pipeline,
                 std::string input_level,
                 std::string input_file,
                 std::string output_file,
                 nlohmann::json parameters);
}