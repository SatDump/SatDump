#pragma once

#include "module.h"

namespace satdump
{
    namespace pipeline
    {
        // TODOREWORK

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
            std::vector<int> normal_live; // Normal CFG, all modules run locally
            std::vector<int> server_live; // Server CFG, modules decode down to packets broadcasted on the network
            std::vector<int> client_live; // Client CFG, the first module subscribes to a server running over the network
            int pkt_size = -1;            // Network packet size, should usually match the frame size (eg, CADU size)
        };

        class Pipeline
        {
        public:
            struct PipelineStep
            {
                std::string level;
                std::string module;
                nlohmann::json parameters;
                std::string input_override;
            };

        public:
            std::string id;
            std::string name;
            nlohmann::json editable_parameters;

        public:
            PipelinePreset preset;
            bool live;
            LivePipelineCfg live_cfg;

        public:
            std::vector<PipelineStep> steps;

        public:
            void run(std::string input_file, std::string output_directory, nlohmann::json parameters, std::string input_level, bool ui = false,
                     std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList = nullptr, std::shared_ptr<std::mutex> uiCallListMutex = nullptr, std::string *final_file = nullptr);

        public:
            static nlohmann::json prepareParameters(nlohmann::json &module_params, nlohmann::json &pipeline_params);
        };

        SATDUMP_DLL extern std::vector<Pipeline> pipelines;
        SATDUMP_DLL extern nlohmann::ordered_json pipelines_json;
        SATDUMP_DLL extern nlohmann::ordered_json pipelines_system_json;

        void loadPipelines(std::string filepath);
        void savePipelines();
        Pipeline getPipelineFromID(std::string pipeline_id);

        namespace events
        {
            struct PipelineDoneProcessingEvent
            {
                std::string pipeline_id;
                std::string output_directory;
            };
        } // namespace events
    } // namespace pipeline
} // namespace satdump