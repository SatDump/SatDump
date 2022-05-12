#pragma once

#include "core/config.h"
#include "imgui/imgui.h"
#include <string>
#include "common/utils.h"
#include "imgui/imgui_stdlib.h"
#include "core/params.h"
#include "imgui/pfd/widget.h"
#include "core/pipeline.h"
#include "common/detect_header.h"

namespace satdump
{
    class PipelineUISelector
    {
    public:
        FileSelectWidget inputfileselect = FileSelectWidget("Input File", "Select Input File");
        FileSelectWidget outputdirselect = FileSelectWidget("Output Directory", "Select Output Directory", true);
        int pipeline_id = 0;
        int pipelines_levels_select_id = -1;

    private:
        bool live_mode;

        std::string pipeline_levels_str;

        std::vector<std::pair<std::string, satdump::params::EditableParameter>> parameters_ui;
        std::vector<std::pair<std::string, satdump::params::EditableParameter>> parameters_ui_pipeline;

        std::string pipeline_search_in;

    public:
        PipelineUISelector(bool live_mode) : live_mode(live_mode)
        {
            nlohmann::ordered_json params = satdump::config::main_cfg["user_interface"]["default_offline_parameters"];

            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : params.items())
                if (!cfg.value().contains("no_live") || !live_mode)
                    parameters_ui.push_back({cfg.key(), satdump::params::EditableParameter(nlohmann::json(cfg.value()))});
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

                if (live_mode)
                {
                    if (it == parameters_ui.end() && cfg.value().contains("type"))
                        parameters_ui_pipeline.push_back({cfg.key(), satdump::params::EditableParameter(cfg.value())});
                    else if (!cfg.value().contains("type") && it != parameters_ui.end())
                        it->second.setValue(cfg.value()["value"]);
                }
                else
                {
                    if (it == parameters_ui.end())
                        parameters_ui_pipeline.push_back({cfg.key(), satdump::params::EditableParameter(cfg.value())});
                    else
                        it->second.setValue(cfg.value()["value"]);
                }
            }

            if (!live_mode)
            {
                pipeline_levels_str = "";
                if (pipeline_id != -1)
                    for (int i = 0; i < (int)pipelines[pipeline_id].steps.size() - 1; i++)
                        pipeline_levels_str += pipelines[pipeline_id].steps[i].level_name + '\0';

                if (pipelines_levels_select_id == -1)
                    pipelines_levels_select_id = 0;
            }
        }

        void renderSelectionBox(double width = -1)
        {
            if (width != -1)
                ImGui::SetNextItemWidth(width);
            ImGui::InputTextWithHint("##pipelinesearchbox", "Search pipelines...", &pipeline_search_in);
            if (width != -1)
                ImGui::SetNextItemWidth(width);
            if (ImGui::BeginListBox("##pipelineslistbox"))
            {
                for (int n = 0; n < (int)pipelines.size(); n++)
                {
                    bool show = true;
                    if (pipeline_search_in.size() != 0)
                        show = isStringPresent(pipelines[n].readable_name, pipeline_search_in);

                    if (show && (!live_mode || pipelines[n].live))
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
        }

        void try_set_param(std::string name, nlohmann::json v)
        {
            for (std::pair<std::string, satdump::params::EditableParameter> &p : parameters_ui)
                if (p.first == name)
                    p.second.setValue(v);

            for (std::pair<std::string, satdump::params::EditableParameter> &p : parameters_ui_pipeline)
                if (p.first == name)
                    p.second.setValue(v);
        }

        void drawMainparams()
        {
            ImGui::BeginTable("##pipelinesmainoptions", 2);
            ImGui::TableSetupColumn("##pipelinesmaincolumn1", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("##pipelinesmaincolumn2", ImGuiTableColumnFlags_WidthStretch, 100);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Input File");
            ImGui::TableSetColumnIndex(1);
            if (inputfileselect.draw())
            {
                std::string file_path = inputfileselect.getPath();

                if (std::filesystem::exists(file_path) && !std::filesystem::is_directory(file_path))
                {
                    HeaderInfo hdr = try_parse_header(file_path);
                    if (hdr.valid)
                    {
                        try_set_param("samplerate", hdr.samplerate);
                        try_set_param("baseband_format", hdr.type);
                    }
                }
            }

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
        }

        void drawMainparamsLive()
        {
            ImGui::Text("Output Directory :");
            outputdirselect.draw();
            ImGui::Spacing();
        }

        void renderParamTable()
        {
            if (ImGui::BeginTable("##pipelineoptions", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                for (std::pair<std::string, satdump::params::EditableParameter> &p : parameters_ui)
                    p.second.draw();
                for (std::pair<std::string, satdump::params::EditableParameter> &p : parameters_ui_pipeline)
                    p.second.draw();
                ImGui::EndTable();
            }
        }

        nlohmann::json getParameters()
        {
            nlohmann::json params2;
            for (std::pair<std::string, satdump::params::EditableParameter> &p : parameters_ui)
                params2[p.first] = p.second.getValue();
            for (std::pair<std::string, satdump::params::EditableParameter> &p : parameters_ui_pipeline)
                params2[p.first] = p.second.getValue();
            return params2;
        }
    };
}