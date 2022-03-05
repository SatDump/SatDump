#include "main_ui.h"
//#include "settingsui.h"
//#include "recorder/recorder_menu.h"
//#include "recorder/recorder.h"
//#include "credits/credits.h"
#include "global.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui.h"
//#include "offline/offline.h"
#include "processing.h"
//#include "live/live.h"
//#include "live/live_run.h"
//#include "projection/projection.h"
//#include "projection/projection_menu.h"
//#include "projection/overlay.h"

// satdump_ui_status satdumpUiStatus = MAIN_MENU;

#include "core/params.h"

#include "nlohmann/json_utils.h"

#include "core/pipeline.h"

nlohmann::ordered_json params = loadJsonFile("cfg.json");

std::map<std::string, satdump::params::EditableParameter> parameters_ui;
std::map<std::string, satdump::params::EditableParameter> parameters_ui_pipeline;

std::string pipelines_str;
int pipeline_id = 0;

void initMainUI()
{
    for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : params.items())
        parameters_ui.insert({cfg.key(), satdump::params::EditableParameter(cfg.value())});
    for (Pipeline pp : pipelines)
        pipelines_str += pp.readable_name + '\0';
}

bool isProcessing = false;

void renderMainUI(int wwidth, int wheight)
{
    if (isProcessing)
    {
        uiCallListMutex->lock();
        float winheight = uiCallList->size() > 0 ? wheight / uiCallList->size() : wheight;
        float currentPos = 0;
        for (std::shared_ptr<ProcessingModule> module : *uiCallList)
        {
            ImGui::SetNextWindowPos({0, currentPos});
            currentPos += winheight;
            ImGui::SetNextWindowSize({(float)wwidth, (float)winheight});
            module->drawUI(false);
        }
        uiCallListMutex->unlock();
    }
    else
    {
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({wwidth, wheight});
        ImGui::Begin("Main", __null, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);

        if (ImGui::Combo("##pipelineselection", &pipeline_id, pipelines_str.c_str()))
        {
            parameters_ui_pipeline.clear();
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> cfg : pipelines[pipeline_id].editable_parameters.items())
                parameters_ui_pipeline.insert({cfg.key(), satdump::params::EditableParameter(cfg.value())});
        }

        if (ImGui::BeginTable("##pipelineoptions", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            for (std::pair<const std::string, satdump::params::EditableParameter> &p : parameters_ui)
                p.second.draw();
            for (std::pair<const std::string, satdump::params::EditableParameter> &p : parameters_ui_pipeline)
                p.second.draw();
            ImGui::EndTable();

            if (ImGui::Button("Show"))
            {
                nlohmann::json params2;
                for (std::pair<const std::string, satdump::params::EditableParameter> &p : parameters_ui)
                    params2[p.first] = p.second.getValue();
                for (std::pair<const std::string, satdump::params::EditableParameter> &p : parameters_ui_pipeline)
                    params2[p.first] = p.second.getValue();
                logger->info(params2.dump(4));
            }
        }

        ImGui::End();
    }
}