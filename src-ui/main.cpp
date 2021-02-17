#include "logger.h"
#include "module.h"
#include "pipeline.h"
#include <signal.h>
#include <filesystem>
#include "nlohmann/json.hpp"
#include <fstream>
#include "render.h"

std::thread renderThread;

int main(int argc, char *argv[])
{
    uiCallList = std::make_shared<std::vector<std::shared_ptr<ProcessingModule>>>();
    uiCallListMutex = std::make_shared<std::mutex>();

// Ignore SIGPIPE
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    mkfifo("test.t", 0777);

    initLogger();

    if (argc < 6)
    {
        logger->info("Usage : " + std::string(argv[0]) + " [downlink] [input_level] [input_file] [output_level] [output_file_or_directory]");
        exit(1);
    }

    std::string downlink_pipeline = argv[1];
    std::string input_level = argv[2];
    std::string input_file = argv[3];
    std::string output_level = argv[4];
    std::string output_file = argv[5];

    std::map<std::string, std::string> parameters;

    if (argc > 6)
    {
        for (int i = 6; i < argc; i++)
        {
            if (i + 1 != argc)
            {
                if (strcmp(argv[i], "-samplerate") == 0) // This is your parameter name
                {
                    parameters.emplace("samplerate", argv[i + 1]); // The next value in the array is your value
                    i++;                                           // Move to the next flag
                }
                else if (strcmp(argv[i], "-baseband_format") == 0) // This is your parameter name
                {
                    parameters.emplace("baseband_format", argv[i + 1]); // The next value in the array is your value
                    i++;                                                // Move to the next flag
                }
            }
        }
    }

    logger->info("   _____       __  ____                      ");
    logger->info("  / ___/____ _/ /_/ __ \\__  ______ ___  ____ ");
    logger->info("  \\__ \\/ __ `/ __/ / / / / / / __ `__ \\/ __ \\");
    logger->info(" ___/ / /_/ / /_/ /_/ / /_/ / / / / / / /_/ /");
    logger->info("/____/\\__,_/\\__/_____/\\__,_/_/ /_/ /_/ .___/ ");
    logger->info("                                    /_/      ");
    logger->info("Starting SatDump v1.0");
    logger->info("");

    registerModules();

    renderThread = std::thread(&render);

    std::vector<Pipeline> pipelines;

    {
        std::ifstream iFstream("../pipelines.json");
        nlohmann::ordered_json jsonObj;
        iFstream >> jsonObj;
        iFstream.close();

        for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> pipelineConfig : jsonObj.items())
        {
            Pipeline newPipeline;
            newPipeline.name = pipelineConfig.key();
            //logger->info(newPipeline.name);

            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> pipelineStep : pipelineConfig.value().items())
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

    logger->debug("Registered pipelines :");
    for (Pipeline &pipeline : pipelines)
        logger->debug(" - " + pipeline.name);

    logger->info("Starting processing pipeline " + downlink_pipeline + "...");
    logger->debug("Input file (" + input_level + ") : " + input_file);
    logger->debug("Output file (" + output_level + ") : " + output_file);

    if (!std::filesystem::exists(output_file))
        std::filesystem::create_directory(output_file);

    std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                      pipelines.end(),
                                                      [&downlink_pipeline](const Pipeline &e) {
                                                          return e.name == downlink_pipeline;
                                                      });

    if (it != pipelines.end())
    {
        it->run(input_file, output_file, parameters, input_level, true, uiCallList, uiCallListMutex);
    }
    else
    {
        logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
    }

    logger->info("Done! Goodbye");
}