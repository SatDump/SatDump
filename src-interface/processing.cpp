#include <filesystem>
#include "processing.h"
#include "logger.h"
#include "core/pipeline.h"
#include "error.h"

namespace satdump
{
    namespace processing
    {
        void process(std::string downlink_pipeline,
                     std::string input_level,
                     std::string input_file,
                     std::string output_file,
                     nlohmann::json parameters)
        {
            is_processing = true;

            logger->info("Starting processing pipeline " + downlink_pipeline + "...");
            logger->debug("Input file (" + input_level + ") : " + input_file);
            logger->debug("Output file : " + output_file);

            if (!std::filesystem::exists(output_file))
                std::filesystem::create_directories(output_file);

            // Get pipeline
            std::optional<Pipeline> pipeline = getPipelineFromName(downlink_pipeline);

            if (pipeline.has_value())
            {
                try
                {
                    pipeline.value().run(input_file, output_file, parameters, input_level);
                }
                catch (std::exception &e)
                {
                    logger->error("Fatal error running pipeline : " + std::string(e.what()));
                    error::set_error("Pipeline Error", e.what());
                    is_processing = false;
                    return;
                }
            }
            else
                logger->critical("Pipeline " + downlink_pipeline + " does not exist!");

            logger->info("Done! Goodbye");
            is_processing = false;
        }

        std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> ui_call_list = std::make_shared<std::vector<std::shared_ptr<ProcessingModule>>>();
        std::shared_ptr<std::mutex> ui_call_list_mutex = std::make_shared<std::mutex>();
        bool is_processing = false;
    }
}