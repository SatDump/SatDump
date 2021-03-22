#include "live.h"
#include "imgui/imgui.h"
#include "global.h"
#include "pipeline.h"
#include <cstring>
#include "logger.h"
#include "portable-file-dialogs.h"
#include "processing.h"
#include <filesystem>

#ifdef BUILD_LIVE

int device_id = 0;

void renderLiveProcessing()
{
    ImGui::BeginGroup();
    {
        std::string names;

        std::map<int, std::string> id_table;

        for (int i = 0, y = 0; i < pipelines.size(); i++)
        {
            if (pipelines[i].live)
            {
                names += pipelines[i].readable_name + '\0';
                id_table.insert({y++, pipelines[i].name});
            }
        }

        ImGui::Text("Pipeline      : ");
        ImGui::SameLine();

        if (ImGui::Combo("##pipeline", &pipeline_id, names.c_str()))
        {
            downlink_pipeline = id_table[pipeline_id];
            std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                              pipelines.end(),
                                                              [](const Pipeline &e) {
                                                                  return e.name == downlink_pipeline;
                                                              });
            std::memcpy(samplerate, std::to_string(it->default_samplerate).c_str(), std::to_string(it->default_samplerate).length());
            frequency = it->frequencies[frequency_id];
        }
        // ImGui::EndCombo();
    }
    ImGui::EndGroup();

    ImGui::BeginGroup();
    {
        ImGui::Text("Output dir : ");
        ImGui::SameLine();

        if (ImGui::Button("Select Output"))
        {
            logger->debug("Opening file dialog");
            auto result = pfd::select_folder("Open output directory", ".");
            while (result.ready(1000))
            {
            }

            if (result.result().size() > 0)
                output_file = result.result();

            logger->debug("Dir " + output_file);
        }

        ImGui::SameLine();
        ImGui::Text(output_file.c_str());
    }
    ImGui::EndGroup();

    ImGui::Separator();

    ImGui::BeginGroup();
    {
        ImGui::Text("Device : ");
        ImGui::SameLine();
        {
            std::string names = ""; // = "baseband\0";
            //std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
            //                                                  pipelines.end(),
            ///                                                  [](const Pipeline &e) {
            //                                                      return e.name == downlink_pipeline;
            //                                                  });

            //if (it != pipelines.end())
            //     for (int i = 0; i < it->frequencies.size(); i++)
            //     {
            //        names += std::to_string(it->frequencies[i]) + " Mhz" + '\0';
            //    }

            if (ImGui::Combo("##device", &device_id, names.c_str()))
            {
                //frequency = it->frequencies[frequency_id];
            }
        }

        ImGui::Text("Frequency : ");
        ImGui::SameLine();
        {
            std::string names; // = "baseband\0";
            std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                              pipelines.end(),
                                                              [](const Pipeline &e) {
                                                                  return e.name == downlink_pipeline;
                                                              });

            if (it != pipelines.end())
                for (int i = 0; i < it->frequencies.size(); i++)
                {
                    names += std::to_string(it->frequencies[i]) + " Mhz" + '\0';
                }

            if (ImGui::Combo("##frequency", &frequency_id, names.c_str()))
            {
                frequency = it->frequencies[frequency_id];
            }
        }

        ImGui::Checkbox("DC Block", &dc_block);

        ImGui::Text("Samplerate (Hz / SPS)");
        ImGui::SameLine();
        ImGui::InputText("##samplerate", samplerate, 100);

        if (ImGui::Button("Start"))
        {
            logger->debug("Starting livedemod...");

            if (output_file == "")
            {
                sprintf(error_message, "Please select an output file!");
            }
            else if (downlink_pipeline == "")
            {
                sprintf(error_message, "Please select a pipeline!");
            }
            else if (frequency <= 0)
            {
                sprintf(error_message, "Please select a frequency!");
            }
            else if (std::stoi(samplerate) <= 0)
            {
                sprintf(error_message, "Please select a samplerate!");
            }
            else
            {
                if (!std::filesystem::exists(output_file))
                    std::filesystem::create_directory(output_file);

                logger->info(downlink_pipeline);

                std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                                  pipelines.end(),
                                                                  [](const Pipeline &e) {
                                                                      return e.name == downlink_pipeline;
                                                                  });

                if (it != pipelines.end())
                {

                    parameters.emplace("samplerate", std::string(samplerate));

                    parameters.emplace("baseband_format", "f32");

                    parameters.emplace("dc_block", dc_block ? "1" : "0");

                    std::map<std::string, std::string> final_parameters = it->steps[1].modules[0].parameters;
                    for (const std::pair<std::string, std::string> &param : parameters)
                        if (final_parameters.count(param.first) > 0)
                            final_parameters[param.first] = param.second;
                        else
                            final_parameters.emplace(param.first, param.second);

                    logger->debug("Parameters :");
                    for (const std::pair<std::string, std::string> &param : final_parameters)
                        logger->debug("   - " + param.first + " : " + param.second);
                }
                else
                {
                    logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
                }
            }
        }

        ImGui::SameLine();

        ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), error_message);
    }
    ImGui::EndGroup();
}
#endif