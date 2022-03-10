#include "offline.h"
#include "imgui/imgui.h"
#include <string>
#include "core/params.h"
#include "core/pipeline.h"
#include "pfd/widget.h"
#include "core/config.h"
#include "processing.h"
#include "main_ui.h"
#include "common/utils.h"

namespace satdump
{
    namespace offline
    {
        FileSelectWidget inputfileselect("Input File", "Select Input File");
        FileSelectWidget outputdirselect("Output Directory", "Select Output Directory", true);

        int pipelines_levels_select_id = -1;
        std::string pipeline_levels_str;

        std::vector<std::pair<std::string, satdump::params::EditableParameter>> parameters_ui;
        std::vector<std::pair<std::string, satdump::params::EditableParameter>> parameters_ui_pipeline;

        char pipeline_search_in[1000];
        int pipeline_id = 0;

        std::string error_message = "";

        void setup()
        {
            nlohmann::ordered_json params = satdump::config::cfg["user_interface"]["default_offline_parameters"];

            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : params.items())
                parameters_ui.push_back({cfg.key(), satdump::params::EditableParameter(cfg.value())});

            memset(pipeline_search_in, 0, 1000);
        }

        void updateSelectedPipeline()
        {
            parameters_ui_pipeline.clear();
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> cfg : pipelines[pipeline_id].editable_parameters.items())
            {
                auto it = std::find_if(parameters_ui.begin(),
                                       parameters_ui.end(),
                                       [&cfg](const std::pair<std::string, satdump::params::EditableParameter> &e)
                                       {
                                           return e.first == cfg.key();
                                       });

                if (it == parameters_ui.end())
                    parameters_ui_pipeline.push_back({cfg.key(), satdump::params::EditableParameter(cfg.value())});
                else
                    it->second.setValue(cfg.value()["value"]);
            }

            pipeline_levels_str = "";
            if (pipeline_id != -1)
                for (int i = 0; i < (int)pipelines[pipeline_id].steps.size() - 1; i++)
                    pipeline_levels_str += pipelines[pipeline_id].steps[i].level_name + '\0';

            if (pipelines_levels_select_id == -1)
                pipelines_levels_select_id = 0;
        }

        void render()
        {
            ImGui::InputTextWithHint("##pipelinesearchbox", "Search pipelines...", pipeline_search_in, 1000);
            if (ImGui::BeginListBox("##pipelineslistbox"))
            {
                for (int n = 0; n < pipelines.size(); n++)
                {
                    bool show = true;
                    std::string search_query(pipeline_search_in);
                    if (search_query.size() != 0)
                        show = isStringPresent(pipelines[n].readable_name, search_query);

                    if (show)
                    {
                        const bool is_selected = (pipeline_id == n);
                        if (ImGui::Selectable(pipelines[n].readable_name.c_str(), is_selected))
                        {
                            pipeline_id = n;
                            updateSelectedPipeline();
                        }

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndListBox();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::BeginTable("##pipelinesmainoptions", 2);
            ImGui::TableSetupColumn("##pipelinesmaincolumn1", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("##pipelinesmaincolumn2", ImGuiTableColumnFlags_WidthStretch, 100);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Input File");
            ImGui::TableSetColumnIndex(1);
            inputfileselect.draw();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Output Directory");
            ImGui::TableSetColumnIndex(1);
            outputdirselect.draw();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Input Level");
            ImGui::TableSetColumnIndex(1);
            ImGui::Combo("##pipelinelevel", &pipelines_levels_select_id, pipeline_levels_str.c_str());
            ImGui::EndTable();

            ImGui::Spacing();
            // ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::BeginTable("##pipelineoptions", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                for (std::pair<std::string, satdump::params::EditableParameter> &p : parameters_ui)
                    p.second.draw();
                for (std::pair<std::string, satdump::params::EditableParameter> &p : parameters_ui_pipeline)
                    p.second.draw();
                ImGui::EndTable();
            }

            if (ImGui::Button("Start"))
            {
                nlohmann::json params2;
                for (std::pair<std::string, satdump::params::EditableParameter> &p : parameters_ui)
                    params2[p.first] = p.second.getValue();
                for (std::pair<std::string, satdump::params::EditableParameter> &p : parameters_ui_pipeline)
                    params2[p.first] = p.second.getValue();

                if (!inputfileselect.file_valid)
                    error_message = "Input file is invalid!";
                else if (!outputdirselect.file_valid)
                    error_message = "Output folder is invalid!";
                else
                    ui_thread_pool.push([&, params2](int)
                                        { processing::process(pipelines[pipeline_id].name,
                                                              pipelines[pipeline_id].steps[pipelines_levels_select_id].level_name,
                                                              inputfileselect.getPath(),
                                                              outputdirselect.getPath(),
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