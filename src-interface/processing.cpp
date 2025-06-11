#include <exception>
#define SATDUMP_DLL_EXPORT2 1
#include "logger.h"
#include "processing.h"
#include <filesystem>

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
        void process(std::string downlink_pipeline, std::string input_level, std::string input_file, std::string output_file, nlohmann::json parameters)
        {
            // Get pipeline
            try
            {
                pipeline::Pipeline pipeline = pipeline::getPipelineFromID(downlink_pipeline);
                process(pipeline, input_level, input_file, output_file, parameters);
            }
            catch (std::exception &e)
            {
                logger->error("Error processing! %s", e.what());
            }
        }

        void process(pipeline::Pipeline downlink_pipeline, std::string input_level, std::string input_file, std::string output_file, nlohmann::json parameters)
        {
            processing_mutex.lock();
            is_processing = true;

            logger->info("Starting processing pipeline " + downlink_pipeline.id + "...");
            logger->debug("Input file (" + input_level + ") : " + input_file);
            logger->debug("Output file : " + output_file);

            if (!std::filesystem::exists(output_file))
                std::filesystem::create_directories(output_file);

            ui_call_list_mutex->lock();
            ui_call_list->clear();
            ui_call_list_mutex->unlock();

            try
            {
                downlink_pipeline.run(input_file, output_file, parameters, input_level, true, ui_call_list, ui_call_list_mutex);
            }
            catch (std::exception &e)
            {
                logger->error("Fatal error running pipeline : " + std::string(e.what()));
                is_processing = false;
                processing_mutex.unlock();
                return;
            }

            is_processing = false;

            logger->info("Done! Goodbye");

            if (satdump_cfg.main_cfg["user_interface"]["open_viewer_post_processing"]["value"].get<bool>())
            {
                if (std::filesystem::exists(output_file + "/dataset.json"))
                {
                    logger->info("Opening viewer!");
                    viewer_app2->tryOpenFileInViewer(output_file + "/dataset.json");
                }
            }

            processing_mutex.unlock();
        }

        SATDUMP_DLL2 std::shared_ptr<std::vector<std::shared_ptr<pipeline::ProcessingModule>>> ui_call_list = std::make_shared<std::vector<std::shared_ptr<pipeline::ProcessingModule>>>();
        SATDUMP_DLL2 std::shared_ptr<std::mutex> ui_call_list_mutex = std::make_shared<std::mutex>();
        SATDUMP_DLL2 bool is_processing = false;
    } // namespace processing
} // namespace satdump