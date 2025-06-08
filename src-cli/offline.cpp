#include "offline.h"
#include "common/cli_utils.h"
#include "common/detect_header.h"
#include "init.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "pipeline/pipeline.h"
#include <filesystem>

int main_offline(int argc, char *argv[])
{
    if (argc < 5) // Check overall command
    {
        logger->error("Usage : " + std::string(argv[0]) + " [pipeline_id] [input_level] [input_file] [output_file_or_directory] [additional options as required]");
        logger->error("Extra options (examples. Any parameter used in modules can be used here) :");
        logger->error(" --samplerate [baseband_samplerate] --baseband_format [cf32/cs16/cs8/cu8] --dc_block --iq_swap");
        logger->error("Sample command :");
        logger->error("./satdump metop_ahrpt baseband /home/user/metop_baseband.cs16 metop_output_directory --samplerate 6e6 --baseband_format s16");
        return 1;
    }

    std::string downlink_pipeline = argv[1];
    std::string input_level = argv[2];
    std::string input_file = argv[3];
    std::string output_file = argv[4];

    // Parse flags
    nlohmann::json parameters = parse_common_flags(argc - 5, &argv[5]);

    try_get_params_from_input_file(parameters, input_file);

    // Init SatDump
    satdump::tle_file_override = parameters.contains("tle_override") ? parameters["tle_override"].get<std::string>() : "";
    satdump::initSatdump();
    completeLoggerInit();

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

    try
    {
        satdump::pipeline::Pipeline pipeline = satdump::pipeline::getPipelineFromID(downlink_pipeline);
        pipeline.run(input_file, output_file, parameters, input_level);
    }
    catch (std::exception &e)
    {
        logger->error("Fatal error running pipeline : " + std::string(e.what()));
        return 1;
    }

    return 0;
}