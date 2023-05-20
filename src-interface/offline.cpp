#include "offline.h"
#include "imgui/imgui.h"
#include <string>
#include "processing.h"
#include "main_ui.h"
#include "pipeline_selector.h"

namespace satdump
{
    namespace offline
    {
        std::unique_ptr<PipelineUISelector> pipeline_selector;

        std::string error_message = "";

        void setup()
        {
            pipeline_selector = std::make_unique<PipelineUISelector>(false);

            pipeline_selector->inputfileselect.default_dir = config::main_cfg["satdump_directories"]["default_input_directory"]["value"].get<std::string>() + "/";
            pipeline_selector->outputdirselect.default_dir = config::main_cfg["satdump_directories"]["default_output_directory"]["value"].get<std::string>() + "/";
        }

        void render()
        {
            pipeline_selector->renderSelectionBox();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            pipeline_selector->drawMainparams();

            ImGui::Spacing();
            // ImGui::Separator();
            ImGui::Spacing();

            pipeline_selector->renderParamTable();

            if (ImGui::Button("Start"))
            {
                nlohmann::json params2 = pipeline_selector->getParameters();

                if (!pipeline_selector->inputfileselect.file_valid)
                    error_message = "Input file is invalid!";
                else if (!pipeline_selector->outputdirselect.file_valid)
                    error_message = "Output folder is invalid!";
                else
                    ui_thread_pool.push([&, params2](int)
                                        { processing::process(pipelines[pipeline_selector->pipeline_id].name,
                                                              pipelines[pipeline_selector->pipeline_id].steps[pipeline_selector->pipelines_levels_select_id].level_name,
                                                              pipeline_selector->inputfileselect.getPath(),
                                                              pipeline_selector->outputdirselect.getPath(),
                                                              params2); });
            }

            if (error_message.size() > 0)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImColor(255, 0, 0), "%s", error_message.c_str());
            }
        }
    }
}