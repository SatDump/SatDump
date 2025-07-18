#include "processing.h"
#include "core/config.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "logger.h"
#include "main_ui.h"
#include <filesystem>
#include <thread>

namespace satdump
{
    namespace handlers
    {
        OffProcessingHandler::OffProcessingHandler(pipeline::Pipeline downlink_pipeline, std::string input_level, std::string input_file, std::string output_file, nlohmann::json parameters)
        {
            ui_call_list = std::make_shared<std::vector<std::shared_ptr<pipeline::ProcessingModule>>>();
            ui_call_list_mutex = std::make_shared<std::mutex>();

            proc_thread = std::thread(&OffProcessingHandler::process, this, downlink_pipeline, input_level, input_file, output_file, parameters);
        }

        OffProcessingHandler::OffProcessingHandler(std::string downlink_pipeline, std::string input_level, std::string input_file, std::string output_file, nlohmann::json parameters)
        {
            ui_call_list = std::make_shared<std::vector<std::shared_ptr<pipeline::ProcessingModule>>>();
            ui_call_list_mutex = std::make_shared<std::mutex>();

            // Get pipeline
            try
            {
                pipeline::Pipeline pipeline = pipeline::getPipelineFromID(downlink_pipeline);
                proc_thread = std::thread(&OffProcessingHandler::process, this, pipeline, input_level, input_file, output_file, parameters);
            }
            catch (std::exception &e)
            {
                logger->error("Error processing! %s", e.what());
            }
        }

        OffProcessingHandler::~OffProcessingHandler()
        {
            if (proc_thread.joinable())
                proc_thread.join();
        }

        void OffProcessingHandler::process(pipeline::Pipeline downlink_pipeline, std::string input_level, std::string input_file, std::string output_file, nlohmann::json parameters)
        {
            eventBus->fire_event<SetIsProcessingEvent>({});

            pipeline_name = downlink_pipeline.name;

            logger->info("Starting processing pipeline " + downlink_pipeline.id + "...");
            logger->debug("Input file (" + input_level + ") : " + input_file);
            logger->debug("Output file : " + output_file);

            if (!std::filesystem::exists(output_file))
                std::filesystem::create_directories(output_file);

            ui_call_list_mutex->lock();
            ui_call_list->clear();
            ui_call_list_mutex->unlock();

            std::string final_file;

            try
            {
                downlink_pipeline.run(input_file, output_file, parameters, input_level, true, ui_call_list, ui_call_list_mutex, &final_file);
            }
            catch (std::exception &e)
            {
                logger->error("Fatal error running pipeline : " + std::string(e.what()));
                return;
            }

            logger->info("Done! Goodbye");

            logger->critical("FINAL FILE : " + final_file); // TODOREWORK!

            // TODOREWORKUI
            if (satdump_cfg.main_cfg["user_interface"]["open_explorer_post_processing"]["value"].get<bool>())
            {
                if (std::filesystem::exists(output_file + "/dataset.json"))
                {
                    logger->info("Opening explorer!");
                    explorer_app->tryOpenFileInExplorer(output_file + "/dataset.json"); // TODOREWORK Maybe just try to open the final file produced?
                }
                else if (std::filesystem::exists(final_file))
                {
                    logger->info("Opening explorer!");
                    explorer_app->tryOpenFileInExplorer(final_file); // TODOREWORK Maybe just try to open the final file produced?
                }
            }

            pipeline_name = "PROCESSING_DONE"; // TODOREWORK MASSIVE HACK!

            eventBus->fire_event<SetIsDoneProcessingEvent>({});
        }

        void OffProcessingHandler::drawMenu() {}

        void OffProcessingHandler::drawContents(ImVec2 win_size)
        {
            auto &dims = win_size;

            float currentPosx = ImGui::GetCursorPos().x;

            // ImGui::BeginChild("OfflineProcessingChild");
            ui_call_list_mutex->lock();

            if (ui_call_list->size())
            {
                int live_width = dims.x; // ImGui::GetWindowWidth();
                int live_height = /*ImGui::GetWindowHeight();*/ dims.y - ImGui::GetCursorPos().y;
                float winheight = ui_call_list->size() > 0 ? live_height / ui_call_list->size() : live_height;
                float currentPos = ImGui::GetCursorPos().y;
                ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive));
                for (std::shared_ptr<pipeline::ProcessingModule> module : *ui_call_list)
                {
                    ImGui::SetNextWindowPos({currentPosx, currentPos});
                    currentPos += winheight;
                    ImGui::SetNextWindowSize({(float)live_width, (float)winheight});
                    module->drawUI(false);
                }
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Text("Done processing! You can close this, if it didn't close itself.");
            }

            ui_call_list_mutex->unlock();
            // ImGui::EndChild();
        }
    } // namespace handlers
} // namespace satdump