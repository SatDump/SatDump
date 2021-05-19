#define SATDUMP_DLL_EXPORT 1
#include "pipeline.h"
#include "logger.h"
#include "module.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <filesystem>

SATDUMP_DLL std::vector<Pipeline> pipelines;
SATDUMP_DLL std::vector<std::string> pipeline_categories;

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

    bool foundLevel = false;

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

        for (PipelineModule modStep : step.modules)
        {
            std::map<std::string, std::string> final_parameters = modStep.parameters;
            for (const std::pair<std::string, std::string> param : parameters)
                if (final_parameters.count(param.first) > 0)
                    ; // Do Nothing //final_parameters[param.first] = param.second;
                else
                    final_parameters.emplace(param.first, param.second);

            logger->debug("Parameters :");
            for (const std::pair<std::string, std::string> &param : final_parameters)
                logger->debug("   - " + param.first + " : " + param.second);

            std::shared_ptr<ProcessingModule> module = modules_registry[modStep.module_name](modStep.input_override == "" ? (stepC == 0 ? input_file : lastFiles[0]) : output_directory + "/" + modStep.input_override, output_directory + "/" + name, final_parameters);

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

            //module.reset();

            std::vector<std::string> newfiles = module->getOutputs();
            files.insert(files.end(), newfiles.begin(), newfiles.end());
        }

        lastFiles = files;
        stepC++;
    }
}

void loadPipeline(std::string filepath, std::string category)
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
        if (newPipeline.live)
            newPipeline.live_cfg = pipelineConfig.value()["live_cfg"].get<std::vector<std::pair<int, int>>>();
        newPipeline.frequencies = pipelineConfig.value()["frequencies"].get<std::vector<float>>();
        newPipeline.default_samplerate = pipelineConfig.value()["samplerate"];
        newPipeline.default_baseband_type = pipelineConfig.value()["baseband_type"];
        newPipeline.category = category;
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

                if (newModule.parameters.count("input_override") > 0)
                    newModule.input_override = newModule.parameters["input_override"];
                else
                    newModule.input_override = "";
                //logger->debug(newModule.module_name);

                newStep.modules.push_back(newModule);
            }

            newPipeline.steps.push_back(newStep);
        }

        pipelines.push_back(newPipeline);
    }
}

void loadPipelines(std::string filepath)
{
    logger->info("Loading pipelines from " + filepath);

    std::vector<std::pair<std::string, std::string>> pipelinesToLoad;

    std::filesystem::recursive_directory_iterator pipelinesIterator(filepath);
    std::error_code iteratorError;
    while (pipelinesIterator != std::filesystem::recursive_directory_iterator())
    {
        if (!std::filesystem::is_directory(pipelinesIterator->path()))
        {
            if (pipelinesIterator->path().filename().string().find(".json") != std::string::npos)
            {
                logger->trace("Found pipeline file " + pipelinesIterator->path().string());
                pipelinesToLoad.push_back({pipelinesIterator->path().string(), pipelinesIterator->path().stem().string()});
            }
        }

        pipelinesIterator.increment(iteratorError);
        if (iteratorError)
            logger->critical(iteratorError.message());
    }

    for (std::pair<std::string, std::string> pipeline : pipelinesToLoad)
    {
        loadPipeline(pipeline.first, pipeline.second);
        pipeline_categories.push_back(pipeline.second);
    }
}

std::vector<Pipeline> getPipelinesInCategory(std::string category)
{
    std::vector<Pipeline> catPipelines;
    std::copy_if(pipelines.begin(), pipelines.end(), std::back_inserter(catPipelines), [=](Pipeline p) { return p.category == category; });
    return catPipelines;
}

Pipeline getPipelineInCategoryFromId(std::string category, int id)
{
    return getPipelinesInCategory(category)[id];
}