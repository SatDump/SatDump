#include "offline.h"
#include "common/widgets/pipeline_selector.h"
#include "common/widgets/timed_message.h"
#include "core/plugin.h"
#include "core/style.h"
#include "explorer/explorer.h"
#include "explorer/processing/processing.h"
#include "imgui/imgui.h"
#include "main_ui.h"
#include <string>

namespace satdump
{
    extern bool offline_en;

    namespace offline
    {
        std::unique_ptr<PipelineUISelector> pipeline_selector;
        widgets::TimedMessage error_message;

        void setup()
        {
            pipeline_selector = std::make_unique<PipelineUISelector>(false);
            pipeline_selector->inputfileselect.setDefaultDir(satdump_cfg.main_cfg["satdump_directories"]["default_input_directory"]["value"].get<std::string>());
            pipeline_selector->outputdirselect.setDefaultDir(satdump_cfg.main_cfg["satdump_directories"]["default_output_directory"]["value"].get<std::string>());
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

                if (!pipeline_selector->inputfileselect.isValid())
                    error_message.set_message(style::theme.red, "Input file is invalid!");
                else if (!pipeline_selector->outputdirselect.isValid())
                    error_message.set_message(style::theme.red, "Output folder is invalid!");
                else
                {
                    eventBus->fire_event<explorer::ExplorerAddHandlerEvent>(
                        {std::make_shared<handlers::OffProcessingHandler>(pipeline_selector->selected_pipeline,
                                                                          pipeline_selector->selected_pipeline.steps[pipeline_selector->pipelines_levels_select_id].level,
                                                                          pipeline_selector->inputfileselect.getPath(), pipeline_selector->outputdirselect.getPath(), params2),
                         true, true});
                    offline_en = false;
                }
            }

            error_message.draw();
        }
    } // namespace offline
} // namespace satdump
