#include "logger.h"
#include "core/module.h"
#include "core/pipeline.h"
#include <signal.h>
#include <filesystem>
#include "nlohmann/json.hpp"
#include <fstream>
#include "init.h"
#include "common/cli_utils.h"

int main(int argc, char *argv[])
{
    // Init logger
    initLogger();

    if (argc < 5) // Check overall command
    {
        logger->error("Usage : " + std::string(argv[0]) + " [downlink] [input_level] [input_file] [output_file_or_directory] [additional options as required]");
        logger->error("Extra options (examples. Any parameter used in modules can be used here) :");
        logger->error(" --samplerate [baseband_samplerate] --baseband_format [f32/i16/i8/w8] --dc_block --iq_swap");
        return 1;
    }

    // Init SatDump
    satdump::initSatdump();

    std::string downlink_pipeline = argv[1];
    std::string input_level = argv[2];
    std::string input_file = argv[3];
    std::string output_file = argv[4];

    // Parse flags
    nlohmann::json parameters = parse_common_flags(argc - 6, &argv[5]);

    // logger->warn("\n" + parameters.dump(4));
    // exit(0);

    // Print some useful info
    logger->info("Starting processing pipeline " + downlink_pipeline + "...");
    logger->debug("Input file (" + input_level + ") : " + input_file);
    logger->debug("Output file : " + output_file);

    // Create output dir
    if (!std::filesystem::exists(output_file))
        std::filesystem::create_directories(output_file);

    // Get pipeline
    std::optional<satdump::Pipeline> pipeline = satdump::getPipelineFromName(downlink_pipeline);

    if (pipeline.has_value())
    {
       // try
        {
            pipeline.value().run(input_file, output_file, parameters, input_level);
        }
       // catch (std::exception &e)
        {
        //    logger->error("Fatal error running pipeline : " + std::string(e.what()));
        //    return 1;
        }
    }
    else
        logger->critical("Pipeline " + downlink_pipeline + " does not exist!");

    logger->info("Done! Goodbye");
}
