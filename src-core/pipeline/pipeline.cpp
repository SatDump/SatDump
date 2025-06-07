#include "core/exception.h"
#define SATDUMP_DLL_EXPORT 1

#include "init.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h" // TODOREWORK
#include "pipeline.h"
#include "pipeline/module.h"
#include <exception>

namespace satdump
{
    namespace pipeline
    {
        void loadPipeline(std::string filepath)
        {
            logger->info("Loading pipelines from file " + filepath);

            // Read file into a string
            std::ifstream fileStream(filepath);
            std::string pipelineString((std::istreambuf_iterator<char>(fileStream)), (std::istreambuf_iterator<char>()));
            fileStream.close();

            // Replace "includes"
            {
                std::map<std::string, std::string> toReplace;
                for (int i = 0; i < int(pipelineString.size() - sizeof(".json.inc")); i++)
                {
                    std::string currentPos = pipelineString.substr(i, 9);

                    if (currentPos == ".json.inc")
                    {
                        int bracketPos = i;
                        for (int y = i; y >= 0; y--)
                        {
                            if (pipelineString[y] == '"')
                            {
                                bracketPos = y;
                                break;
                            }
                        }

                        std::string finalStr = pipelineString.substr(bracketPos, (i - bracketPos) + 10);
                        std::string filenameToLoad = finalStr.substr(1, finalStr.size() - 2);
                        std::string pathToLoad = std::filesystem::path(filepath).parent_path().string() + "/" + filenameToLoad;

                        if (std::filesystem::exists(pathToLoad))
                        {
                            std::ifstream fileStream(pathToLoad);
                            std::string includeString((std::istreambuf_iterator<char>(fileStream)), (std::istreambuf_iterator<char>()));
                            fileStream.close();

                            toReplace.emplace(finalStr, includeString);
                        }
                        else
                        {
                            logger->error("Could not include " + pathToLoad + "!");
                        }
                    }
                }

                for (std::pair<std::string, std::string> replace : toReplace)
                {
                    while (pipelineString.find(replace.first) != std::string::npos)
                        pipelineString.replace(pipelineString.find(replace.first), replace.first.size(), replace.second);
                }

                // logger->info(pipelineString);
            }

            try
            {
                pipelines_system_json.update(nlohmann::ordered_json::parse(pipelineString));
            }
            catch (std::exception &e)
            {
                logger->warn("Error loading system pipeline file: %s", e.what());
            }
        }

        void parsePipelines()
        {
            pipelines.clear();
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> pipelineConfig : pipelines_json.items())
            {
                try
                {
                    Pipeline newPipeline;

                    // Parse basics
                    newPipeline.id = pipelineConfig.key();
                    newPipeline.name = pipelineConfig.value()["name"];
                    newPipeline.editable_parameters = pipelineConfig.value()["parameters"];

#if 1
                    // Parse live configuration if preset
                    newPipeline.live = pipelineConfig.value().contains("live");
                    if (newPipeline.live)
                    {
                        try
                        {
                            newPipeline.live_cfg.normal_live = pipelineConfig.value()["live"].get<std::vector<int>>();
                        }
                        catch (std::exception &)
                        {
                            newPipeline.live_cfg.normal_live = pipelineConfig.value()["live"]["default"].get<std::vector<int>>();
                            if (pipelineConfig.value()["live_cfg"].contains("server"))
                                newPipeline.live_cfg.server_live = pipelineConfig.value()["live"]["server"].get<std::vector<int>>();
                            if (pipelineConfig.value()["live_cfg"].contains("client"))
                                newPipeline.live_cfg.client_live = pipelineConfig.value()["live"]["client"].get<std::vector<int>>();
                            if (pipelineConfig.value()["live_cfg"].contains("pkt_size"))
                                newPipeline.live_cfg.pkt_size = pipelineConfig.value()["live_cfg"]["pkt_size"].get<int>();
                        }
                    }

                    // Parse and set presets
                    if (newPipeline.editable_parameters.contains("samplerate"))
                    { // We attempt to get a preset samplerate
                        if (newPipeline.editable_parameters["samplerate"].contains("value"))
                            newPipeline.preset.samplerate = newPipeline.editable_parameters["samplerate"]["value"];
                        else
                            newPipeline.preset.samplerate = 0;
                    }
                    if (pipelineConfig.value().contains("frequencies"))
                        newPipeline.preset.frequencies = pipelineConfig.value()["frequencies"].get<std::vector<std::pair<std::string, uint64_t>>>();
#endif // TODOREWORK

                    // logger->info(newPipeline.name);

                    bool hasAtLeastOneModule = false;
                    bool hasAllModules = true;

                    for (auto &pipelineStep : pipelineConfig.value()["work"].items())
                    {
                        Pipeline::PipelineStep newStep;
                        newStep.level = pipelineStep.key();
                        // logger->warn(newStep.level_name);

                        if (pipelineStep.value().contains("module"))
                        {
                            newStep.module = pipelineStep.value()["module"];
                            if (pipelineStep.value().contains("parameters"))
                                newStep.parameters = pipelineStep.value()["parameters"];

                            if (newStep.parameters.count("input_override") > 0)
                                newStep.input_override = newStep.parameters["input_override"];
                            else
                                newStep.input_override = "";
                            // logger->debug(newModule.module_name);

                            if (!moduleExists(newStep.module) && hasAllModules)
                            {
                                logger->warn("Module " + newStep.module + " is not loaded. Skipping pipeline!");
                                hasAllModules = false;
                            }

                            hasAtLeastOneModule = true;
                        }

                        newPipeline.steps.push_back(newStep);
                    }

                    if (!hasAtLeastOneModule)
                        logger->warn("Pipeline " + newPipeline.id + " has no modules!");

                    if (hasAllModules && hasAtLeastOneModule)
                        pipelines.push_back(newPipeline);
                }
                catch (std::exception &e)
                {
                    logger->error("Couldn't load pipeline (%s)! %s", pipelineConfig.key().c_str(), e.what());
                }
            }

            std::sort(pipelines.begin(), pipelines.end(),
                      [](const Pipeline &l, const Pipeline &r)
                      {
                          std::string lname = l.name;
                          std::string rname = r.name;
                          std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);
                          std::transform(rname.begin(), rname.end(), rname.begin(), ::tolower);
                          return lname < rname;
                      });
        }

        // TODOREWORK
        SATDUMP_DLL std::vector<Pipeline> pipelines;
        SATDUMP_DLL nlohmann::ordered_json pipelines_json;
        SATDUMP_DLL nlohmann::ordered_json pipelines_system_json;
        std::string user_cfg_path;

        void loadPipelines(std::string filepath)
        {
            if (!std::filesystem::exists(filepath))
            {
                logger->error("Couldn't load pipelines! Was trying : " + filepath);
                exit(1);
            }

            logger->info("Loading system pipelines from " + filepath);

            std::vector<std::string> systemPipelines;
            std::filesystem::recursive_directory_iterator pipelinesIterator(filepath);
            std::error_code iteratorError;
            while (pipelinesIterator != std::filesystem::recursive_directory_iterator())
            {
                if (!std::filesystem::is_directory(pipelinesIterator->path()))
                {
                    if (pipelinesIterator->path().filename().string().find(".json") != std::string::npos)
                    {
                        if (pipelinesIterator->path().string().find(".json.inc") == std::string::npos)
                        {
                            logger->trace("Found system pipeline file " + pipelinesIterator->path().string());
                            systemPipelines.push_back(pipelinesIterator->path().string());
                        }
                    }
                }

                pipelinesIterator.increment(iteratorError);
                if (iteratorError)
                    logger->critical(iteratorError.message());
            }

            std::sort(systemPipelines.begin(), systemPipelines.end());
            for (std::string &pipeline : systemPipelines)
                loadPipeline(pipeline);

            // Add User Pipelines
            nlohmann::ordered_json user_pipelines;
            bool has_user_pipelines = false;
            std::string final_path = "";

            if (std::filesystem::exists("pipelines.json")) // First try loading in current folder
                final_path = "pipelines.json";
            else if (std::filesystem::exists(user_path + "/pipelines.json"))
                final_path = user_path + "/pipelines.json";

            if (final_path != "")
            {
                logger->info("Found user pipelines " + final_path);
                user_cfg_path = final_path;
                has_user_pipelines = true;
                try
                {
                    pipelines_json = merge_json_diffs(pipelines_system_json, loadJsonFile(user_cfg_path));
                }
                catch (std::exception &e)
                {
                    logger->warn("Error loading user pipelines: %s", e.what());
                    has_user_pipelines = false;
                }
            }
            else
            {
                user_cfg_path = user_path + "/pipelines.json";
            }

            if (!has_user_pipelines)
                pipelines_json = pipelines_system_json;

            parsePipelines();
        }

        void savePipelines()
        {
            // Check edited pipelines are valid
            try
            {
                parsePipelines();
            }
            catch (std::exception &e)
            {
                logger->error("Error parsing customized pipelines! Resetting to last good config\n\n%s", e.what());
                pipelines_json = merge_json_diffs(pipelines_system_json, loadJsonFile(user_cfg_path));
                parsePipelines();
            }

            // Save Pipelines
            nlohmann::ordered_json diff_json = perform_json_diff(pipelines_system_json, pipelines_json);
            try
            {
                if (!std::filesystem::exists(std::filesystem::path(user_cfg_path).parent_path()) && std::filesystem::path(user_cfg_path).has_parent_path())
                    std::filesystem::create_directories(std::filesystem::path(user_cfg_path).parent_path());
            }
            catch (std::exception &e)
            {
                logger->error("Cannot create directory for user pipelines: %s", e.what());
                return;
            }

            logger->info("Saving user pipelines at " + user_cfg_path);
            saveJsonFile(user_cfg_path, diff_json);
        }

        Pipeline getPipelineFromID(std::string pipeline_id)
        {
            std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(), pipelines.end(), [&pipeline_id](const Pipeline &e) { return e.id == pipeline_id; });

            if (it != pipelines.end())
                return *it;

            throw satdump_exception("Pipeline " + pipeline_id + " not found!");
        }
    } // namespace pipeline
} // namespace satdump