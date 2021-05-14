#pragma once

#include <mutex>
#include <memory>
#include "module.h"
#include "pipeline.h"

extern std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList;
extern std::shared_ptr<std::mutex> uiCallListMutex;

extern bool processing;

void process(std::string downlink_pipeline,
             std::string input_level,
             std::string input_file,
             std::string output_level,
             std::string output_file,
             std::map<std::string, std::string> parameters);