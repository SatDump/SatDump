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
    // Basic Baseband-level "proposed / recommended" presets
    struct PipelinePreset
    {
        bool exists = false;
        uint64_t samplerate;                                       // Samplerate to come close to, preferrably above rather than below
        std::vector<std::pair<std::string, uint64_t>> frequencies; // Preset frequencies
    };

    // Live configurations
    struct LivePipelineCfg
    {
        std::vector<std::pair<int, int>> normal_live; // Normal CFG, all modules run locally
        std::vector<std::pair<int, int>> server_live; // Server CFG, modules decode down to packets broadcasted on the network
        std::vector<std::pair<int, int>> client_live; // Client CFG, the first module subscribes to a server running over the network
        int pkt_size = -1;                            // Network packet size, should usually match the frame size (eg, CADU size)
    };

    struct Pipeline
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

        std::string name = "";
        std::string readable_name;

        PipelinePreset preset;

        bool live;
        LivePipelineCfg live_cfg;

        nlohmann::json editable_parameters;

        std::vector<PipelineStep> steps;
        void run(std::string input_file,
                 std::string output_directory,
                 nlohmann::json parameters,
                 std::string input_level,
                 bool ui = false,
                 std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList = nullptr,
                 std::shared_ptr<std::mutex> uiCallListMutex = nullptr);

        static nlohmann::json prepareParameters(nlohmann::json &module_params, nlohmann::json &pipeline_params);
    };

    SATDUMP_DLL extern std::vector<Pipeline> pipelines;
    SATDUMP_DLL extern nlohmann::ordered_json pipelines_json;
    SATDUMP_DLL extern nlohmann::ordered_json pipelines_system_json;

    void loadPipelines(std::string filepath);
    void savePipelines();
    std::optional<Pipeline> getPipelineFromName(std::string downlink_pipeline);

    namespace events
    {
        struct PipelineDoneProcessingEvent
        {
            std::string pipeline_id;
            std::string output_directory;
        };
    }
}