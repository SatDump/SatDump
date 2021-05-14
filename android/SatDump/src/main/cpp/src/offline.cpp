#include "offline.h"
#include "imgui/imgui.h"
#include "global.h"
#include "pipeline.h"
#include <cstring>
#include "logger.h"
//#include "portable-file-dialogs.h"
#include "processing.h"
#include "settings.h"

std::string getFilePath();
std::string getDirPath();

void renderOfflineProcessing()
{
    ImGui::BeginGroup();
    {
        std::string names;

        for (int i = 0; i < pipeline_categories.size(); i++)
        {
            names += pipeline_categories[i] + '\0';
        }

        ImGui::Text("Category    : ");
        ImGui::SameLine();

        if (ImGui::Combo("##category", &category_id, names.c_str()))
        {
        }
    }
    ImGui::EndGroup();

    std::vector<Pipeline> catPipelines = getPipelinesInCategory(pipeline_categories[category_id]);

    ImGui::BeginGroup();
    {
        std::string names;

        for (int i = 0; i < catPipelines.size(); i++)
        {
            names += catPipelines[i].readable_name + '\0';
        }

        ImGui::Text("Pipeline      : ");
        ImGui::SameLine();

        if (ImGui::Combo("##pipeline", &pipeline_id, names.c_str()))
        {
            downlink_pipeline = catPipelines[pipeline_id].name;
            std::memcpy(samplerate, std::to_string(catPipelines[pipeline_id].default_samplerate).c_str(), std::to_string(catPipelines[pipeline_id].default_samplerate).length());

            if (catPipelines[pipeline_id].default_baseband_type == "f32")
            {
                baseband_type_option = 0;
            }
            else if (catPipelines[pipeline_id].default_baseband_type == "i8")
            {
                baseband_type_option = 2;
            }
            else if (catPipelines[pipeline_id].default_baseband_type == "i16")
            {
                baseband_type_option = 1;
            }
            else if (catPipelines[pipeline_id].default_baseband_type == "w8")
            {
                baseband_type_option = 3;
            }
            baseband_format = catPipelines[pipeline_id].default_baseband_type;

            input_level_id = 0;

            input_level = catPipelines[pipeline_id].steps[input_level_id].level_name;
        }
    }
    ImGui::EndGroup();

    ImGui::BeginGroup();
    {
        ImGui::Text("Input Level : ");
        ImGui::SameLine();

        // logger->info(names);

        std::string names; // = "baseband\0";

        if (pipeline_id != -1)
            for (int i = 0; i < catPipelines[pipeline_id].steps.size(); i++)
            {
                names += catPipelines[pipeline_id].steps[i].level_name + '\0';
            }

        if (ImGui::Combo("##input_level", &input_level_id, names.c_str()))
        {
            input_level = catPipelines[pipeline_id].steps[input_level_id].level_name;
        }
    }
    ImGui::EndGroup();

    ImGui::BeginGroup();
    {
        ImGui::Text("Input File : ");
        ImGui::SameLine();

        if (ImGui::Button("Select Input"))
        {
            logger->debug("Opening file dialog");
            //auto result = pfd::open_file("Open input file", ".", {".*"}, false);
            // while (result.ready(1000))
            // {
            // }

            // if (result.result().size() > 0)
            //    input_file = result.result()[0];
            input_file = getFilePath();

            logger->debug("Dir " + input_file);
        }

        ImGui::SameLine();
        ImGui::Text("%s", input_file.c_str());
    }
    ImGui::EndGroup();

    ImGui::BeginGroup();
    {
        ImGui::Text("Output dir : ");
        ImGui::SameLine();

        if (ImGui::Button("Select Output"))
        {
            logger->debug("Opening file dialog");
            //auto result = pfd::select_folder("Open output directory", ".");
            //while (result.ready(1000))
            //{
            //}

            //if (result.result().size() > 0)
            //    output_file = result.result();

            output_file = getDirPath();

            logger->debug("Dir " + output_file);
        }

        ImGui::SameLine();
        ImGui::Text("%s", output_file.c_str());
    }
    ImGui::EndGroup();

    ImGui::BeginGroup();
    {
        ImGui::Text("Parameters : ");

        if (ImGui::BeginTable("table1", 2, ImGuiTableFlags_Borders))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Samplerate (Hz / SPS)");
            ImGui::TableSetColumnIndex(1);
            ImGui::InputText("  ", samplerate, 100);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Baseband Type");
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Combo("", &baseband_type_option, "f32\0i16\0i8\0w8\0"))
            {
                switch (baseband_type_option)
                {
                case 0:
                    baseband_format = "f32";
                    break;
                case 1:
                    baseband_format = "i16";
                    break;
                case 2:
                    baseband_format = "i8";
                    break;
                case 3:
                    baseband_format = "w8";
                    break;
                }
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("DC Block");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("DC Block", &dc_block);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("IQ Swap (baseband file)");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("IQ Swap", &iq_swap);

            ImGui::EndTable();
        }
    }
    ImGui::EndGroup();

    ImGui::BeginGroup();
    {
        if (ImGui::Button("Start"))
        {
            logger->debug("Starting...");

            // Save settings
            //{
            //    settings["offline_pipeline_id"] = pipeline_id;
            //    saveSettings();
            //}

            if (input_file == "")
            {
                sprintf(error_message, "Please select an input file!");
            }
            else if (output_file == "")
            {
                sprintf(error_message, "Please select an output file!");
            }
            else if (downlink_pipeline == "")
            {
                sprintf(error_message, "Please select a pipeline!");
            }
            else if (input_level == "")
            {
                sprintf(error_message, "Please select an input level!");
            }
            else
            {

                parameters.emplace("samplerate", std::string(samplerate));

                parameters.emplace("baseband_format", baseband_format);

                parameters.emplace("dc_block", dc_block ? "1" : "0");
                parameters.emplace("iq_swap", iq_swap ? "1" : "0");

                processThreadPool.push([&](int) { process(downlink_pipeline, input_level, input_file, output_level, output_file, parameters); });
                //showStartup = false;
            }
        }

        ImGui::SameLine();

        ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), "%s", error_message);
    }
    ImGui::EndGroup();
}