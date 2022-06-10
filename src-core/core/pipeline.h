#pragma once

#include <string>
#include "core/module.h"
#include <mutex>
#include <vector>
#include "dll_export.h"
#include "nlohmann/json.hpp"
#include <optional>

namespace satdump
{
    struct PipelineModule
    {
        std::string module_name;
        nlohmann::json parameters;
        std::string input_override;
    };

    struct PipelineStep
    {
        std::string level_name;
        std::vector<PipelineModule> modules;
    };

    // Basic Baseband-level "proposed / recommended" presets
    struct PipelinePreset
    {
        bool exists = false;
        uint64_t samplerate;                                       // Samplerate to come close to, preferrably above rather than below
        std::vector<std::pair<std::string, uint64_t>> frequencies; // Preset frequencies
    };

    struct Pipeline
    {
        std::string name;
        std::string readable_name;

        PipelinePreset preset;

        bool live;
        std::vector<std::pair<int, int>> live_cfg;

        nlohmann::json editable_parameters;

        std::vector<PipelineStep> steps;
        void run(std::string input_file,
                 std::string output_directory,
                 nlohmann::json parameters,
                 std::string input_level,
                 bool ui = false,
                 std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList = nullptr,
                 std::shared_ptr<std::mutex> uiCallListMutex = nullptr);

        nlohmann::json prepareParameters(nlohmann::json &module_params, nlohmann::json &pipeline_params);
    };

    SATDUMP_DLL extern std::vector<Pipeline> pipelines;

    void loadPipelines(std::string filepath);
    std::optional<Pipeline> getPipelineFromName(std::string downlink_pipeline);
}