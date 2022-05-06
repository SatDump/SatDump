#include "logger.h"
#include "core/module.h"
#include "core/pipeline.h"
#include <signal.h>
#include <filesystem>
#include "nlohmann/json.hpp"
#include <fstream>
#include "init.h"
#include "common/cli_utils.h"

#include "common/dsp_sample_source/dsp_sample_source.h"
#include "live_pipeline.h"

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

    if (std::string(argv[1]) == "live")
    {
        std::string downlink_pipeline = argv[2];
        std::string output_file = argv[3];

        // Parse flags
        nlohmann::json parameters = parse_common_flags(argc - 4, &argv[4]);

        uint64_t samplerate = parameters["samplerate"].get<uint64_t>();
        uint64_t frequency = parameters["frequency"].get<uint64_t>();
        uint64_t timeout = parameters.contains("timeout") ? parameters["timeout"].get<uint64_t>() : 0;
        std::string handler_id = parameters["source"].get<std::string>();

        dsp::registerAllSources();

        std::vector<dsp::SourceDescriptor> source_tr = dsp::getAllAvailableSources();
        dsp::SourceDescriptor selected_src;

        for (dsp::SourceDescriptor src : source_tr)
            logger->debug("Device " + src.name);

        for (dsp::SourceDescriptor src : source_tr)
        {
            if (handler_id == src.source_type)
                selected_src = src;
        }

        std::shared_ptr<dsp::DSPSampleSource> source_ptr = getSourceFromDescriptor(selected_src);
        source_ptr->open();
        source_ptr->set_frequency(frequency);
        source_ptr->set_samplerate(samplerate);
        source_ptr->set_settings(parameters);

        // Get pipeline
        std::optional<satdump::Pipeline> pipeline = satdump::getPipelineFromName(downlink_pipeline);
        parameters["baseband_format"] = "f32";
        parameters["buffer_size"] = STREAM_BUFFER_SIZE; // This is required, as we WILL go over the (usually) default 8192 size
        std::unique_ptr<satdump::LivePipeline> live_pipeline = std::make_unique<satdump::LivePipeline>(pipeline.value(), parameters, output_file);

        ctpl::thread_pool ui_thread_pool(8);
        source_ptr->start();
        live_pipeline->start(source_ptr->output_stream, ui_thread_pool);

        uint64_t start_time = time(0);

        while (1)
        {
            if (timeout > 0)
            {
                int elapsed_time = time(0) - start_time;
                if (elapsed_time >= timeout)
                    break;
            }
        }

        source_ptr->stop();
        live_pipeline->stop();
    }
    else
    {
        std::string downlink_pipeline = argv[1];
        std::string input_level = argv[2];
        std::string input_file = argv[3];
        std::string output_file = argv[4];

        // Parse flags
        nlohmann::json parameters = parse_common_flags(argc - 5, &argv[5]);

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
            try
            {
                pipeline.value().run(input_file, output_file, parameters, input_level);
            }
            catch (std::exception &e)
            {
                logger->error("Fatal error running pipeline : " + std::string(e.what()));
                return 1;
            }
        }
        else
            logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
    }

    logger->info("Done! Goodbye");
}
