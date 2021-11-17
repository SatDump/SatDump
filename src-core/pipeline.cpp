#define SATDUMP_DLL_EXPORT 1
#include "pipeline.h"
#include "logger.h"
#include "module.h"
#include <fstream>
#include <filesystem>

SATDUMP_DLL std::vector<Pipeline> pipelines;
SATDUMP_DLL std::vector<std::string> pipeline_categories;

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
            nlohmann::json final_parameters = modStep.parameters;
            for (const nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> &param : parameters.items())
                if (final_parameters.count(param.key()) > 0)
                    ; // Do Nothing //final_parameters[param.first] = param.second;
                else
                    final_parameters.emplace(param.key(), param.value());

            logger->debug("Parameters :");
            for (const nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> &param : final_parameters.items())
                logger->debug("   - " + param.key() + " : " + param.value().dump());

            // Check module exists!
            if (modules_registry.count(modStep.module_name) <= 0)
            {
                logger->critical("Module " + modStep.module_name + " is not registered. Cancelling pipeline.");
                return;
            }

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
                std::string pathToLoad = std::filesystem::path(filepath).parent_path().relative_path().string() + "/" + filenameToLoad;

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

        //logger->info(pipelineString);
    }

    // Parse it
    nlohmann::ordered_json jsonObj = nlohmann::ordered_json::parse(pipelineString);

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
                newModule.parameters = pipelineModule.value();

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
                if (pipelinesIterator->path().string().find(".json.inc") != std::string::npos)
                {
                }
                else
                {
                    logger->trace("Found pipeline file " + pipelinesIterator->path().string());
                    pipelinesToLoad.push_back({pipelinesIterator->path().string(), pipelinesIterator->path().stem().string()});
                }
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
    std::copy_if(pipelines.begin(), pipelines.end(), std::back_inserter(catPipelines), [=](Pipeline p)
                 { return p.category == category; });
    return catPipelines;
}

Pipeline getPipelineInCategoryFromId(std::string category, int id)
{
    return getPipelinesInCategory(category)[id];
}
