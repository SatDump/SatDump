#include <filesystem>
#include "processing.h"
#include "logger.h"
#include "core/pipeline.h"
#include "error.h"

#include "core/config.h"
#include "main_ui.h"

namespace satdump
{
    // TMP, MOVE TO HEADER
    extern std::shared_ptr<Application> current_app;
    extern bool in_app;

    std::mutex processing_mutex;

    namespace processing
    {
        void process(std::string downlink_pipeline,
                     std::string input_level,
                     std::string input_file,
                     std::string output_file,
                     nlohmann::json parameters)
        {
            processing_mutex.lock();
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
                    pipeline.value().run(input_file, output_file, parameters, input_level, true, ui_call_list, ui_call_list_mutex);
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

            is_processing = false;

            logger->info("Done! Goodbye");

            if (config::main_cfg["user_interface"]["open_viewer_post_processing"]["value"].get<bool>())
            {
                if (std::filesystem::exists(output_file + "/dataset.json"))
                {
                    logger->info("Opening viewer!");
                    viewer_app->loadDatasetInViewer(output_file + "/dataset.json");
                }
            }
            processing_mutex.unlock();
        }

        std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> ui_call_list = std::make_shared<std::vector<std::shared_ptr<ProcessingModule>>>();
        std::shared_ptr<std::mutex> ui_call_list_mutex = std::make_shared<std::mutex>();
        bool is_processing = false;
    }
}