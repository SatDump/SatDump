#define SATDUMP_DLL_EXPORT 1
#include "pipeline.h"
#include "logger.h"
#include "core/module.h"
#include <fstream>
#include <filesystem>
#include <thread>
#include "core/config.h"
#include "core/exception.h"
#include "init.h"
#include "nlohmann/json_utils.h"

namespace satdump
{
    SATDUMP_DLL std::vector<Pipeline> pipelines;
    SATDUMP_DLL nlohmann::ordered_json pipelines_json;
    SATDUMP_DLL nlohmann::ordered_json pipelines_system_json;
    std::string user_cfg_path;

    void Pipeline::run(std::string input_file,
                       std::string output_directory,
                       nlohmann::json parameters,
                       std::string input_level,
                       bool ui,
                       std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList,
                       std::shared_ptr<std::mutex> uiCallListMutex)
    {
        if (!std::filesystem::exists(input_file))
        {
            logger->error("Input file " + input_file + " does not exist!");
            return;
        }

        logger->debug("Starting " + name);

        std::vector<std::string> lastFiles;

        int currentStep = 0;
        int stepC = 0;
        bool foundLevel = false;

        /*
        In most cases, all processing pipelines will start from
        baseband, demodulate to some intermediate level and then
        feed an actual decoder down to frames.

        Hence, in most cases, if the first and second module both
        support streaming data from the first to the second, we
        can skip this intermediate level and do both in parrallel.

        Here, we first test modules are compatible with this way
        of doing things, then unless specifically disabled by the
        user, proceed to run both in parallel saving up on processing
        time.
        */
        if (input_level == "baseband" &&
            parameters.count("disable_multi_modules") == 0 &&
            steps.size() >= 3 &&
            steps[1].modules.size() == 1 &&
            steps[2].modules.size() == 1)
        {
            logger->info("Checking the 2 first modules...");

            PipelineModule module1 = steps[1].modules[0];
            PipelineModule module2 = steps[2].modules[0];

            if (modules_registry.count(module1.module_name) <= 0 || modules_registry.count(module2.module_name) <= 0)
                throw satdump_exception("Module " + module1.module_name + " or " + module2.module_name + " is not registered. Cancelling pipeline.");

            nlohmann::json params1 = prepareParameters(module1.parameters, parameters);
            nlohmann::json params2 = prepareParameters(module2.parameters, parameters);

            std::shared_ptr<ProcessingModule> m1 = modules_registry[module1.module_name](module1.input_override == "" ? input_file : output_directory + "/" + module1.input_override,
                                                                                         output_directory + "/" + name,
                                                                                         params1);
            std::shared_ptr<ProcessingModule> m2 = modules_registry[module2.module_name](module2.input_override == "" ? input_file : output_directory + "/" + module2.input_override,
                                                                                         output_directory + "/" + name,
                                                                                         params2);

            std::vector<ModuleDataType> m1_outputs = m1->getOutputTypes();
            std::vector<ModuleDataType> m2_inputs = m2->getInputTypes();

            bool m1_has_stream = std::find(m1_outputs.begin(), m1_outputs.end(), DATA_STREAM) != m1_outputs.end();
            bool m2_has_stream = std::find(m2_inputs.begin(), m2_inputs.end(), DATA_STREAM) != m2_inputs.end();

            if (m1_has_stream && m2_has_stream)
            {
                logger->info("Both 2 first modules can be run at once!");

                m1->setInputType(DATA_FILE);
                m1->setOutputType(DATA_STREAM);
                m1->output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);

                m2->input_fifo = m1->output_fifo;
                m2->setInputType(DATA_STREAM);
                m2->setOutputType(DATA_FILE);
                m2->input_active = true;

                m1->init();
                m2->init();

                if (ui)
                {
                    uiCallListMutex->lock();
                    uiCallList->push_back(m1);
                    uiCallList->push_back(m2);
                    uiCallListMutex->unlock();
                }

                std::thread module1_thread([&m1]()
                                           { m1->process(); });
                std::thread module2_thread([&m2]()
                                           { m2->process(); });

                if (module1_thread.joinable())
                    module1_thread.join();
                while (m2->input_fifo->getReadable() > 0)
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                m2->input_active = false;
                m2->input_fifo->stopReader();
                m2->input_fifo->stopWriter();
                m2->stop();
                if (module2_thread.joinable())
                    module2_thread.join();

                if (ui)
                {
                    uiCallListMutex->lock();
                    uiCallList->clear();
                    uiCallListMutex->unlock();
                }

                lastFiles = m2->getOutputs();
                currentStep += 2;
                input_level = steps[2].level_name;
                stepC++;
            }
        }

        for (; currentStep < (int)steps.size(); currentStep++)
        {
            PipelineStep &step = steps[currentStep];

            if (!foundLevel)
            {
                foundLevel = step.level_name == input_level;
                logger->info("Data is already at level " + step.level_name + ", skipping");
                continue;
            }

            logger->info("Processing data to level " + step.level_name);

            std::vector<std::string> files;

            for (PipelineModule modStep : step.modules)
            {
                // Check module exists!
                if (modules_registry.count(modStep.module_name) <= 0)
                    throw satdump_exception("Module " + modStep.module_name + " is not registered. Cancelling pipeline.");

                nlohmann::json final_parameters = prepareParameters(modStep.parameters, parameters);

                std::shared_ptr<ProcessingModule> module = modules_registry[modStep.module_name](modStep.input_override == "" ? (stepC == 0 ? input_file : lastFiles[0]) : output_directory + "/" + modStep.input_override,
                                                                                                 output_directory + "/" + name,
                                                                                                 final_parameters);

                module->setInputType(DATA_FILE);
                module->setOutputType(DATA_FILE);

                module->init();

                if (ui)
                {
                    uiCallListMutex->lock();
                    uiCallList->push_back(module);
                    uiCallListMutex->unlock();
                }

                module->process();

                if (ui)
                {
                    uiCallListMutex->lock();
                    uiCallList->clear();
                    uiCallListMutex->unlock();
                }

                std::vector<std::string> newfiles = module->getOutputs();
                files.insert(files.end(), newfiles.begin(), newfiles.end());

                module.reset();
            }

            lastFiles = files;
            stepC++;
        }

        // We are done. Does this have a dataset?
        if (std::filesystem::exists(output_directory + "/dataset.json") &&
            config::main_cfg["satdump_general"]["auto_process_products"]["value"].get<bool>())
        {
            logger->debug("Products processing is enabled! Starting processing module.");

            // It does, fire up the processing module.
            std::shared_ptr<ProcessingModule> module = modules_registry["products_processor"](output_directory + "/dataset.json",
                                                                                              output_directory + "/" + name,
                                                                                              "");

            module->setInputType(DATA_FILE);
            module->setOutputType(DATA_FILE);

            module->init();

            if (ui)
            {
                uiCallListMutex->lock();
                uiCallList->push_back(module);
                uiCallListMutex->unlock();
            }

            module->process();

            if (ui)
            {
                uiCallListMutex->lock();
                uiCallList->clear();
                uiCallListMutex->unlock();
            }

            module.reset();
        }

        satdump::eventBus->fire_event<events::PipelineDoneProcessingEvent>({name, output_directory});
    }

    nlohmann::json Pipeline::prepareParameters(nlohmann::json &module_params, nlohmann::json &pipeline_params)
    {
        nlohmann::json final_parameters = module_params;
        for (const nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> &param : pipeline_params.items())
            if (final_parameters.count(param.key()) > 0)
                final_parameters[param.key()] = param.value();
            else
                final_parameters.emplace(param.key(), param.value());

        logger->debug("Parameters :");
        for (const nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> &param : final_parameters.items())
            logger->debug("   - " + param.key() + " : " + param.value().dump());

        return final_parameters;
    }

    void loadPipeline(std::string filepath)
    {
        logger->info("Loading pipelines from file " + filepath);

        // Read file into a string
        std::ifstream fileStream(filepath);
        std::string pipelineString((std::istreambuf_iterator<char>(fileStream)),
                                   (std::istreambuf_iterator<char>()));
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
                        std::string includeString((std::istreambuf_iterator<char>(fileStream)),
                                                  (std::istreambuf_iterator<char>()));
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
            Pipeline newPipeline;

            // Parse basics
            newPipeline.name = pipelineConfig.key();
            newPipeline.readable_name = pipelineConfig.value()["name"];
            newPipeline.editable_parameters = pipelineConfig.value()["parameters"];

            // Parse live configuration if preset
            newPipeline.live = pipelineConfig.value().contains("live") ? pipelineConfig.value()["live"].get<bool>() : false;
            if (newPipeline.live)
            {
                try
                {
                    newPipeline.live_cfg.normal_live = pipelineConfig.value()["live_cfg"].get<std::vector<std::pair<int, int>>>();
                }
                catch (std::exception &)
                {
                    newPipeline.live_cfg.normal_live = pipelineConfig.value()["live_cfg"]["default"].get<std::vector<std::pair<int, int>>>();
                    if (pipelineConfig.value()["live_cfg"].contains("server"))
                        newPipeline.live_cfg.server_live = pipelineConfig.value()["live_cfg"]["server"].get<std::vector<std::pair<int, int>>>();
                    if (pipelineConfig.value()["live_cfg"].contains("client"))
                        newPipeline.live_cfg.client_live = pipelineConfig.value()["live_cfg"]["client"].get<std::vector<std::pair<int, int>>>();
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

            // logger->info(newPipeline.name);

            bool hasAllModules = true;

            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> pipelineStep : pipelineConfig.value()["work"].items())
            {
                Pipeline::PipelineStep newStep;
                newStep.level_name = pipelineStep.key();
                // logger->warn(newStep.level_name);

                for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> pipelineModule : pipelineStep.value().items())
                {
                    Pipeline::PipelineModule newModule;
                    newModule.module_name = pipelineModule.key();
                    newModule.parameters = pipelineModule.value();

                    if (newModule.parameters.count("input_override") > 0)
                        newModule.input_override = newModule.parameters["input_override"];
                    else
                        newModule.input_override = "";
                    // logger->debug(newModule.module_name);

                    if (modules_registry.count(newModule.module_name) <= 0 && hasAllModules)
                    {
                        logger->warn("Module " + newModule.module_name + " is not loaded. Skipping pipeline!");
                        hasAllModules = false;
                    }

                    newStep.modules.push_back(newModule);
                }

                newPipeline.steps.push_back(newStep);
            }

            if (hasAllModules)
                pipelines.push_back(newPipeline);
        }

        std::sort(pipelines.begin(), pipelines.end(), [](const Pipeline &l, const Pipeline &r)
                  {
                                                    std::string lname = l.readable_name;
                                                    std::string rname = r.readable_name;
                                                    std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);
                                                    std::transform(rname.begin(), rname.end(), rname.begin(), ::tolower);
                                                    return lname < rname; });
    }

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
            if (!std::filesystem::exists(std::filesystem::path(user_cfg_path).parent_path()) &&
                std::filesystem::path(user_cfg_path).has_parent_path())
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

    std::optional<Pipeline> getPipelineFromName(std::string downlink_pipeline)
    {
        std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                          pipelines.end(),
                                                          [&downlink_pipeline](const Pipeline &e)
                                                          {
                                                              return e.name == downlink_pipeline;
                                                          });

        if (it != pipelines.end())
            return std::optional<Pipeline>(*it);
        else
            return std::optional<Pipeline>();
    }
}
