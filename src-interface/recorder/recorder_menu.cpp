#include "recorder.h"

#ifdef BUILD_LIVE
#include "imgui/imgui.h"
#include "global.h"
#include "logger.h"
#include "sdr/sdr.h"
#include "settings.h"
#include "settingsui.h"
#include "main_ui.h"

namespace recorder
{
    std::shared_ptr<SDRDevice> radio;
    int device_id = 0;
    std::map<std::string, std::string> device_parameters;

    char frequency_field[100];
    char samplerate_field[100];

    int sample_format = 1;

    void initRecorderMenu()
    {
        std::fill(frequency_field, &frequency_field[100], 0);
        std::fill(samplerate_field, &samplerate_field[0], 0);

        if (settings.count("recorder_format") > 0)
            sample_format = settings["recorder_format"].get<int>();
    }

    void renderRecorderMenu()
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

        ImGui::Text("Frequency (MHz)");
        ImGui::SameLine();
        ImGui::InputText("##frequency", frequency_field, 100);

        ImGui::Text("Samplerate (Hz / SPS)");
        ImGui::SameLine();
        ImGui::InputText("##samplerate", samplerate_field, 100);

        ImGui::Text("Format");
        ImGui::SameLine();
        if (ImGui::Combo("##recorderformat", &sample_format, "i8\0"
                                                             "i16\0"
                                                             "f32\0"
#ifdef BUILD_ZIQ
                                                             "ZIQ Compressed 8-bits\0"
                                                             "ZIQ Compressed 16-bits\0"
                                                             "ZIQ Compressed 32-bits (float)\0"
#endif
                         ))
        {
        }

        if (ImGui::Button("Start"))
        {
            logger->debug("Starting recorder...");

            logger->debug("Starting SDR...");

            std::string devID = getDeviceIDStringByID(radio_devices, device_id);

            std::map<std::string, std::string> settings_parameters = settings["recorder_sdr"].count(devID) > 0 ? settings["recorder_sdr"][devID].get<std::map<std::string, std::string>>() : std::map<std::string, std::string>();
            for (const std::pair<std::string, std::string> param : settings_parameters)
                if (device_parameters.count(param.first) > 0)
                    ; // Do Nothing
                else
                    device_parameters.emplace(param.first, param.second);

            logger->debug("Device parameters " + devID + ":");
            for (const std::pair<std::string, std::string> param : device_parameters)
                logger->debug("   - " + param.first + " : " + param.second);

            radio = getDeviceByID(radio_devices, device_parameters, device_id);

            try
            {
                radio->setFrequency(std::stof(frequency_field) * 1e6);
            }
            catch (std::exception &e)
            {
                radio->setFrequency(0);
            }
            try
            {
                radio->setSamplerate(std::stoi(samplerate_field));
            }
            catch (std::exception &e)
            {
                logger->critical("Invalid samplerate");
            }
            radio->start();

            initRecorder();
            satdumpUiStatus = BASEBAND_RECORDER;

            settings["recorder_format"] = sample_format;
            saveSettings();
        }
    }
};
#endif