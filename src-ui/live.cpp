#include "live.h"

#ifdef BUILD_LIVE
#include "imgui/imgui.h"
#include "global.h"
#include "pipeline.h"
#include <cstring>
#include "logger.h"
#include "portable-file-dialogs.h"
#include "processing.h"
#include <filesystem>
#include "sdr/sdr.h"
#include "live_run.h"
#include "sdr/sdr.h"
std::shared_ptr<SDRDevice> radio;
std::vector<std::shared_ptr<ProcessingModule>> liveModules;

int device_id = 0;
std::map<std::string, std::string> device_parameters;

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devices;

void initLive()
{
    initSDRs();
    devices = getAllDevices();
}

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
            std::string names; // = "baseband\0";

            for (int i = 0; i < devices.size(); i++)
            {
                names += std::get<0>(devices[i]) + "" + '\0';
            }

            if (ImGui::Combo("##device", &device_id, names.c_str()))
            {
                //frequency = it->frequencies[frequency_id];
            }
        }

        device_parameters = drawParamsUIForID(devices, device_id);

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

                    for (std::pair<int, int> currentModule : it->live_cfg)
                    {
                        std::map<std::string, std::string> final_parameters = it->steps[currentModule.first].modules[currentModule.second].parameters;
                        for (const std::pair<std::string, std::string> &param : parameters)
                            if (final_parameters.count(param.first) > 0)
                                final_parameters[param.first] = param.second;
                            else
                                final_parameters.emplace(param.first, param.second);

                        logger->debug("Parameters :");
                        for (const std::pair<std::string, std::string> &param : final_parameters)
                            logger->debug("   - " + param.first + " : " + param.second);

                        liveModules.push_back(modules_registry[it->steps[currentModule.first]
                                                                   .modules[currentModule.second]
                                                                   .module_name]("", output_file + "/" + it->name, final_parameters));
                    }

                    logger->debug("Starting SDR...");
                    radio = getDeviceByID(devices, device_parameters, device_id);
                    radio->setFrequency(frequency * 1e6);
                    radio->setSamplerate(std::stoi(samplerate));
                    radio->start();
                    startRealLive();

                    live_processing = true;
                }
                else
                {
                    logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
                }
            }
        }

        ImGui::SameLine();

        ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), error_message);

        ImGui::Text("Please keep in mind live processing support in SatDump is still Work-In-Progress. \n"
                    "While it should be perfectly functional and work as expected, bugs and crashes could still happen... \n"
                    "Please report if any of those occur! Anyway, enjoy! :-)");
        ImGui::Text(" - Aang23");
        ImGui::Text("PS : About saving settings between SatDump restarts, this is planned and will be implemented \nin the future. (Soon enough, it's in current priorities)");
        ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), "There current is NO automatic samplerate selection \n"
                                                               "system that takes into account what the selected device can actually do. \n"
                                                               "Please ensure the selected samplerate is correct before pressing start.");
    }
    ImGui::EndGroup();
}
#endif