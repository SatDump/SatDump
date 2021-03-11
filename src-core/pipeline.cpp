#define SATDUMP_DLL_EXPORT 1
#include "pipeline.h"
#include "logger.h"
#include "module.h"
#include "nlohmann/json.hpp"
#include <fstream>

SATDUMP_DLL std::vector<Pipeline> pipelines;

void Pipeline::run(std::string input_file,
                   std::string output_directory,
                   std::map<std::string, std::string> parameters,
                   std::string input_level,
                   bool ui,
                   std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList,
                   std::shared_ptr<std::mutex> uiCallListMutex)
{
    logger->debug("Starting " + name);

    std::vector<std::string> lastFiles;

    int stepC = 0;

    bool foundLevel;

    for (PipelineStep &step : steps)
    {
        if (!foundLevel)
        {
            foundLevel = step.level_name == input_level;
            logger->warn("Data is already at level " + step.level_name + ", skipping");
            continue;
        }

        logger->warn("Processing data to level " + step.level_name);

        std::vector<std::string> files;

        for (PipelineModule &modStep : step.modules)
        {
            std::map<std::string, std::string> final_parameters = modStep.parameters;
            for (const std::pair<std::string, std::string> &param : parameters)
                if (final_parameters.count(param.first) > 0)
                    ; // Do Nothing //final_parameters[param.first] = param.second;
                else
                    final_parameters.emplace(param.first, param.second);

            logger->debug("Parameters :");
            for (const std::pair<std::string, std::string> &param : final_parameters)
                logger->debug("   - " + param.first + " : " + param.second);

            std::shared_ptr<ProcessingModule> module = modules_registry[modStep.module_name](stepC == 0 ? input_file : lastFiles[0], output_directory + "/" + name, final_parameters);

            module->setInputType(DATA_FILE);
            module->setOutputType(DATA_FILE);

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

            //module.reset();

            std::vector<std::string> newfiles = module->getOutputs();
            files.insert(files.end(), newfiles.begin(), newfiles.end());
        }

        lastFiles = files;
        stepC++;
    }
}

void loadPipelines(std::string filepath)
{
    logger->info("Loading pipelines from file " + filepath);

    std::ifstream iFstream(filepath);
    nlohmann::ordered_json jsonObj;
    iFstream >> jsonObj;
    iFstream.close();

    for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> pipelineConfig : jsonObj.items())
    {
        Pipeline newPipeline;
        newPipeline.name = pipelineConfig.key();
        newPipeline.readable_name = pipelineConfig.value()["name"];
        newPipeline.live = pipelineConfig.value()["live"];
        newPipeline.frequencies = pipelineConfig.value()["frequencies"].get<std::vector<float>>();
        newPipeline.default_samplerate = pipelineConfig.value()["samplerate"];
        newPipeline.default_baseband_type = pipelineConfig.value()["baseband_type"];
        //logger->info(newPipeline.name);

        for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> pipelineStep : pipelineConfig.value()["work"].items())
        {
            PipelineStep newStep;
            newStep.level_name = pipelineStep.key();
            //logger->warn(newStep.level_name);

            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> pipelineModule : pipelineStep.value().items())
            {
                PipelineModule newModule;
                newModule.module_name = pipelineModule.key();
                newModule.parameters = pipelineModule.value().get<std::map<std::string, std::string>>();
                //logger->debug(newModule.module_name);

                newStep.modules.push_back(newModule);
            }

            newPipeline.steps.push_back(newStep);
        }

        pipelines.push_back(newPipeline);
    }
}