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

#include <filesystem>
#include "pfd/portable-file-dialogs.h"

struct FileSelectWidget
{
    std::string label;
    std::string selection_text;
    std::string id;
    std::string btnid;

    char path[10000];
    bool file_valid;

    bool directory;

    void draw()
    {
        bool is_dir = std::filesystem::is_directory(path);
        file_valid = std::filesystem::exists(path) && (directory ? is_dir : !is_dir);

        if (!file_valid)
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        ImGui::InputText(id.c_str(), path, 10000);
        if (!file_valid)
            ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button(btnid.c_str()))
        {
            if (!directory)
            {
                auto fileselect = pfd::open_file(selection_text.c_str(), ".", {}, false);

                while (!fileselect.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (fileselect.result().size() > 0)
                {
                    memset(path, 0, 10000);
                    memcpy(path, fileselect.result()[0].c_str(), fileselect.result()[0].size());
                }
            }
            else
            {
                auto dirselect = pfd::select_folder(selection_text.c_str(), ".");

                while (!dirselect.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (dirselect.result().size() > 0)
                {
                    memset(path, 0, 10000);
                    memcpy(path, dirselect.result().c_str(), dirselect.result().size());
                }
            }
        }
    }

    FileSelectWidget(std::string label, std::string selection_text, bool directory = false)
        : label(label), selection_text(selection_text), directory(directory)
    {
        id = "##filepathselection" + label;
        btnid = "Select##filepathselectionbutton" + label;
    }

    std::string getPath()
    {
        return std::string(path);
    }
};

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

FileSelectWidget inputfileselect("Input File", "Select Input File");
FileSelectWidget outputdirselect("Output Directory", "Select Output Directory", true);

int pipelines_levels_select_id = -1;
std::string pipeline_levels_str;

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

        if (ImGui::Combo("##pipelineselection", &pipeline_id, pipelines_str.c_str()) || pipelines_levels_select_id == -1)
        {
            parameters_ui_pipeline.clear();
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> cfg : pipelines[pipeline_id].editable_parameters.items())
            {
                if (parameters_ui.count(cfg.key()) <= 0)
                    parameters_ui_pipeline.insert({cfg.key(), satdump::params::EditableParameter(cfg.value())});
                else
                    parameters_ui[cfg.key()].setValue(cfg.value()["value"]);
            }

            pipeline_levels_str = "";
            if (pipeline_id != -1)
                for (int i = 0; i < (int)pipelines[pipeline_id].steps.size(); i++)
                    pipeline_levels_str += pipelines[pipeline_id].steps[i].level_name + '\0';

            if (pipelines_levels_select_id == -1)
                pipelines_levels_select_id = 0;
        }

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

        if (ImGui::BeginTable("##pipelineoptions", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            for (std::pair<const std::string, satdump::params::EditableParameter> &p : parameters_ui)
                p.second.draw();
            for (std::pair<const std::string, satdump::params::EditableParameter> &p : parameters_ui_pipeline)
                p.second.draw();
            ImGui::EndTable();

            if (ImGui::Button("Start"))
            {
                nlohmann::json params2;
                for (std::pair<const std::string, satdump::params::EditableParameter> &p : parameters_ui)
                    params2[p.first] = p.second.getValue();
                for (std::pair<const std::string, satdump::params::EditableParameter> &p : parameters_ui_pipeline)
                    params2[p.first] = p.second.getValue();
                logger->info(params2.dump(4));

                processThreadPool.push([&, params2](int)
                                       { processing::process(pipelines[pipeline_id].name,
                                                             pipelines[pipeline_id].steps[pipelines_levels_select_id].level_name,
                                                             inputfileselect.getPath(),
                                                             outputdirselect.getPath(),
                                                             params2); });
            }
        }

        ImGui::End();
    }
}