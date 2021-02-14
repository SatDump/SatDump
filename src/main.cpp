#include "logger.h"
#include "module.h"
#include "pipeline.h"
#include <signal.h>
#include <filesystem>

#include "pipelines.h"

int main(int argc, char *argv[])
{
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

    logger->info("Starting processing pipeline " + downlink_pipeline + "...");
    logger->debug("Input file (" + input_level + ") : " + input_file);
    logger->debug("Output file (" + output_level + ") : " + output_file);

    if (!std::filesystem::exists(output_file))
        std::filesystem::create_directory(output_file);

    if (downlink_pipeline == "metop_ahrpt")
        metop_ahrpt.run(input_file, output_file, parameters, input_level);
    else if (downlink_pipeline == "fengyun3_ab_ahrpt")
        fengyun3ab_ahrpt.run(input_file, output_file, parameters, input_level);
    else if (downlink_pipeline == "fengyun3_c_ahrpt")
        fengyun3c_ahrpt.run(input_file, output_file, parameters, input_level);
    else if (downlink_pipeline == "fengyun3_abc_mpt")
        fengyun3abc_mpt.run(input_file, output_file, parameters, input_level);
    else if (downlink_pipeline == "fengyun3_d_ahrpt")
        fengyun3d_ahrpt.run(input_file, output_file, parameters, input_level);
    else if (downlink_pipeline == "aqua_db")
        aqua_db.run(input_file, output_file, parameters, input_level);
    else if (downlink_pipeline == "noaa_hrpt")
        noaa_hrpt.run(input_file, output_file, parameters, input_level);

    logger->info("Done! Goodbye");
}