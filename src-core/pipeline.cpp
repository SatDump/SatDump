#include "pipeline.h"
#include "logger.h"
#include "module.h"

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
                    final_parameters[param.first] = param.second;
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

            std::vector<std::string> newfiles = module->getOutputs();
            files.insert(files.end(), newfiles.begin(), newfiles.end());
        }

        lastFiles = files;
        stepC++;
    }
}