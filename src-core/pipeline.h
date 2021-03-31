#pragma once

#include <string>
#include "module.h"
#include <mutex>
#include <vector>
#include "dll_export.h"

struct PipelineModule
{
    std::string module_name;
    std::map<std::string, std::string> parameters;
};

struct PipelineStep
{
    std::string level_name;
    std::vector<PipelineModule> modules;
};

struct Pipeline
{
    std::string name;
    std::string readable_name;
    bool live;
    std::vector<float> frequencies;
    long default_samplerate;
    std::string default_baseband_type;
    std::vector<std::pair<int, int>> live_cfg;

    std::vector<PipelineStep> steps;
    void run(std::string input_file,
             std::string output_directory,
             std::map<std::string, std::string> parameters,
             std::string input_level,
             bool ui = false,
             std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList = nullptr,
             std::shared_ptr<std::mutex> uiCallListMutex = nullptr);
};

SATDUMP_DLL extern std::vector<Pipeline> pipelines;

void loadPipelines(std::string filepath);
