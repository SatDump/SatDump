#include "offline.h"
#include "imgui/imgui.h"
#include "pipeline.h"
#include <cstring>
#include "logger.h"
#include "imgui/file_selection.h"
#include "processing.h"
#include "settings.h"
#include "global.h"
#include "common/wav.h"
#ifdef BUILD_ZIQ
#include "common/ziq.h"
#endif

namespace offline
{
    // Variables
    int category_id = 0;
    int pipeline_id = -1;
    int input_level_id = 0;

    std::string downlink_pipeline = "";
    std::string input_level = "";
    std::string input_file = "";
    std::string output_level = "products";
    std::string output_file = "";

    char error_message[100];

    std::map<std::string, std::string> parameters;

    char samplerate[100];
    int baseband_type_option = 2;
    std::string baseband_format = "";
    bool dc_block = false;
    bool iq_swap = false;

    void initOfflineProcessingMenu()
    {
        std::fill(samplerate, &samplerate[0], 0);
        std::fill(error_message, &error_message[0], 0);
    }

    void renderOfflineProcessingMenu()
    {
        ImGui::BeginGroup();
        {
            std::string names;

            for (int i = 0; i < (int)pipeline_categories.size(); i++)
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

            for (int i = 0; i < (int)catPipelines.size(); i++)
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

            std::string names;
            if (pipeline_id != -1)
            {
                for (int i = 0; i < (int)catPipelines[pipeline_id].steps.size(); i++)
                {
                    names += catPipelines[pipeline_id].steps[i].level_name + '\0';
                }
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

                std::string file = selectInputFileDialog("Select Input File", ".", {".*"});

                if (file.size() > 0)
                    input_file = file;

                logger->debug("Dir " + input_file);

                // Try to parse wav header
                if (input_level == "baseband")
                {
                    wav::WavHeader header = wav::parseHeaderFromFileWav(input_file);

                    if (wav::isValidWav(header))
                    {
                        logger->debug("This file is WAV, parsing header...");
                        memset(samplerate, 0, sizeof(samplerate));
                        memcpy(samplerate, std::to_string(header.samplerate).c_str(), std::to_string(header.samplerate).size());
                        switch (header.sub_chunk_size)
                        {
                        case 8:
                            baseband_type_option = 3;
                            break;
                        case 16:
                            baseband_type_option = 1;
                            break;
                        case 18:
                            baseband_type_option = 0;
                            break;
                        }
                    }
                    else if (wav::isValidRF64(header))
                    {
                        wav::RF64Header header = wav::parseHeaderFromFileRF64(input_file);
                        logger->debug("This file is RF64, parsing header...");
                        memset(samplerate, 0, sizeof(samplerate));
                        memcpy(samplerate, std::to_string(header.samplerate).c_str(), std::to_string(header.samplerate).size());
                        switch (header.sub_chunk_size)
                        {
                        case 8:
                            baseband_type_option = 3;
                            break;
                        case 16:
                            baseband_type_option = 1;
                            break;
                        case 18:
                            baseband_type_option = 0;
                            break;
                        }
                    }
#ifdef BUILD_ZIQ
                    else if (ziq::isValidZIQ(input_file))
                    {
                        ziq::ziq_cfg header = ziq::getCfgFromFile(input_file);
                        logger->debug("This file is ZIQ, parsing header...");
                        memset(samplerate, 0, sizeof(samplerate));
                        memcpy(samplerate, std::to_string(header.samplerate).c_str(), std::to_string(header.samplerate).size());
                        baseband_type_option = 4;
                    }
#endif

                    // Ensure we select the format properly
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
#ifdef BUILD_ZIQ
                    case 4:
                        baseband_format = "ziq";
                        break;
#endif
                    }
                }
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

                std::string dir = selectDirectoryDialog("Select output directory", ".");

                if (dir.size() > 0)
                    output_file = dir;

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
                if (ImGui::Combo("", &baseband_type_option, "f32\0"
                                                            "i16\0"
                                                            "i8\0"
                                                            "w8\0"
#ifdef BUILD_ZIQ
                                                            "ZIQ\0"
#endif
                                 ))
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
#ifdef BUILD_ZIQ
                    case 4:
                        baseband_format = "ziq";
                        break;
#endif
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

                    processThreadPool.push([&](int)
                                           { processing::process(downlink_pipeline, input_level, input_file, output_level, output_file, parameters); });
                }
            }

            ImGui::SameLine();

            ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), "%s", error_message);
        }
        ImGui::EndGroup();
    }
}