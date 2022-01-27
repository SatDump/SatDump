#include "live.h"

#ifdef BUILD_LIVE
#include "imgui/imgui.h"
#include "global.h"
#include "pipeline.h"
#include <cstring>
#include "logger.h"
#include "imgui/file_selection.h"
#include "processing.h"
#include <filesystem>
#include "sdr/sdr.h"
#include "live_run.h"
#include "sdr/sdr.h"
#include "settings.h"
#include "live_pipeline.h"
#include "settingsui.h"
#include "main_ui.h"

namespace live
{
    // Variables
    std::string downlink_pipeline = "";
    int pipeline_id = -1;
    int frequency_id = 0;
    float frequency;
    char samplerate[100];
    char error_message[100];
    std::string output_level = "products";
    std::string output_file = "";
    bool dc_block = false;
    bool iq_swap = false;
    nlohmann::json parameters;

    void initLiveProcessingMenu()
    {
        std::fill(samplerate, &samplerate[0], 0);
        std::fill(error_message, &error_message[0], 0);
    }

    std::shared_ptr<SDRDevice> radio;
    extern std::shared_ptr<dsp::stream<std::complex<float>>> moduleStream;
    std::shared_ptr<LivePipeline> live_pipeline;

    int device_id = 0;
    std::map<std::string, std::string> device_parameters;

    void renderLiveProcessingMenu()
    {
        ImGui::BeginGroup();
        {
            std::string names;

            std::map<int, std::string> id_table;

            for (int i = 0, y = 0; i < (int)pipelines.size(); i++)
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
                                                                  [](const Pipeline &e)
                                                                  {
                                                                      return e.name == downlink_pipeline;
                                                                  });
                std::memcpy(samplerate, std::to_string(it->default_samplerate).c_str(), std::to_string(it->default_samplerate).length());
                frequency = it->frequencies[frequency_id];
            }
            // ImGui::EndCombo();
        }
        ImGui::EndGroup();

        if (!live_use_generated_output_folder)
        {
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
        }
        else
        {
            ImGui::Text("Output directory will be automically created. You can disable this in Settings.");
        }

        ImGui::Separator();

        ImGui::BeginGroup();
        {
            ImGui::Text("Device : ");
            ImGui::SameLine();
            {
                std::string names; // = "baseband\0";

                for (int i = 0; i < (int)radio_devices.size(); i++)
                {
                    names += std::get<0>(radio_devices[i]) + "" + '\0';
                }

                if (ImGui::Combo("##device", &device_id, names.c_str()))
                {
                    //frequency = it->frequencies[frequency_id];
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh"))
            {
                findRadioDevices();
            }

            device_parameters = drawParamsUIForID(radio_devices, device_id);

            ImGui::Text("Frequency : ");
            ImGui::SameLine();
            {
                std::string names; // = "baseband\0";
                std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                                  pipelines.end(),
                                                                  [](const Pipeline &e)
                                                                  {
                                                                      return e.name == downlink_pipeline;
                                                                  });

                if (it != pipelines.end())
                    for (int i = 0; i < (int)it->frequencies.size(); i++)
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

                // Generate filename
                if (live_use_generated_output_folder)
                {
                    const time_t timevalue = time(0);
                    std::tm *timeReadable = gmtime(&timevalue);
                    std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                                            (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                                            (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                                            (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                                            (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));

                    output_file = default_live_output_folder + "/" + timestamp + "_" + downlink_pipeline + "_" + std::to_string(long(frequency)) + "Mhz";
                    logger->info("Generated folder name : " + output_file);
                }

                if (output_file == "" && !live_use_generated_output_folder)
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
                        std::filesystem::create_directories(output_file);

                    logger->info(downlink_pipeline);

                    std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                                      pipelines.end(),
                                                                      [](const Pipeline &e)
                                                                      {
                                                                          return e.name == downlink_pipeline;
                                                                      });

                    if (it != pipelines.end())
                    {
                        parameters.clear();
                        parameters["samplerate"] = std::stol(samplerate);
                        parameters["baseband_format"] = "f32";
                        parameters["dc_block"] = (bool)dc_block;

                        live_pipeline = std::make_shared<LivePipeline>(*it, parameters, output_file);

                        logger->debug("Starting SDR...");

                        std::string devID = getDeviceIDStringByID(radio_devices, device_id);

                        std::map<std::string, std::string> settings_parameters = settings["sdr"].count(devID) > 0 ? settings["sdr"][devID].get<std::map<std::string, std::string>>() : std::map<std::string, std::string>();
                        for (const std::pair<std::string, std::string> param : settings_parameters)
                            if (device_parameters.count(param.first) > 0)
                                ; // Do Nothing
                            else
                                device_parameters.emplace(param.first, param.second);

                        logger->debug("Device parameters " + devID + ":");
                        for (const std::pair<std::string, std::string> param : device_parameters)
                            logger->debug("   - " + param.first + " : " + param.second);

                        radio = getDeviceByID(radio_devices, device_parameters, device_id);

                        radio->setFrequency(frequency * 1e6);
                        radio->setSamplerate(std::stoi(samplerate));
                        radio->start();
                        startRealLive();

                        satdumpUiStatus = LIVE_PROCESSING;
                    }
                    else
                    {
                        logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
                    }
                }
            }

            ImGui::SameLine();

            ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), "%s", error_message);

            ImGui::Text("Please keep in mind live processing support in SatDump is still Work-In-Progress. \n"
                        "While it should be perfectly functional and work as expected, bugs and crashes could still happen... \n"
                        "Please report if any of those occur! Anyway, enjoy! :-)");
            ImGui::Text(" - Aang23");
            ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), "There current is NO automatic samplerate selection \n"
                                                                   "system that takes into account what the selected device can actually do. \n"
                                                                   "Please ensure the selected samplerate is correct before pressing start.");
        }
        ImGui::EndGroup();
    }
};
#endif