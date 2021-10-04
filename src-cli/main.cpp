#include "logger.h"
#include "module.h"
#include "pipeline.h"
#include <signal.h>
#include <filesystem>
#include "nlohmann/json.hpp"
#include <fstream>
#include "init.h"
#include "project.h"

int main(int argc, char *argv[])
{
// Ignore SIGPIPE
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    initLogger();

    if (strcmp(argv[1], "project") == 0)
    {
        initSatdump();
        return project(argc - 1, &argv[1]);
    }
    else if (argc < 6 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        logger->info("Usage : " + std::string(argv[0]) + " [downlink] [input_level] [input_file] [output_level] [output_file_or_directory] [additional options as required]");
        logger->info("Extra options : -samplerate [baseband_samplerate] -baseband_format [f32/i16/i8/w8] -dc_block -iq_swap");
        exit(1);
    }

    initSatdump();

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
                else if (strcmp(argv[i], "-dc_block") == 0) // This is your parameter name
                {
                    parameters.emplace("dc_block", argv[i + 1]); // The next value in the array is your value
                    i++;                                         // Move to the next flag
                }
                else if (strcmp(argv[i], "-iq_swap") == 0) // This is your parameter name
                {
                    parameters.emplace("iq_swap", argv[i + 1]); // The next value in the array is your value
                    i++;                                        // Move to the next flag
                }
            }
        }
    }

    logger->info("Starting processing pipeline " + downlink_pipeline + "...");
    logger->debug("Input file (" + input_level + ") : " + input_file);
    logger->debug("Output file (" + output_level + ") : " + output_file);

    if (!std::filesystem::exists(output_file))
        std::filesystem::create_directory(output_file);

    std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                      pipelines.end(),
                                                      [&downlink_pipeline](const Pipeline &e)
                                                      {
                                                          return e.name == downlink_pipeline;
                                                      });

    if (it != pipelines.end())
    {
        it->run(input_file, output_file, parameters, input_level);
    }
    else
    {
        logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
    }

    logger->info("Done! Goodbye");
}
