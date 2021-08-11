#include "module_noaa_extractor.h"
#include "logger.h"
#include "imgui/imgui.h"

size_t getFilesize(std::string filepath);

#define BUFFER_SIZE 11090 * 2

namespace noaa
{
    NOAAExtractorModule::NOAAExtractorModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        buffer = new uint16_t[BUFFER_SIZE / 2];
        frameBuffer = new uint8_t[104];
        windowTitle = "NOAA Frame Extractor";
    }

    NOAAExtractorModule::~NOAAExtractorModule()
    {
        delete[] buffer;
        delete[] frameBuffer;
    }

    void NOAAExtractorModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        tip_out = std::ofstream(d_output_file_hint + ".tip", std::ios::binary);
        aip_out = std::ofstream(d_output_file_hint + ".aip", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".tip");
        d_output_files.push_back(d_output_file_hint + ".aip");

        time_t lastTime = 0;

        logger->info("Using input data " + d_input_file);
        logger->info("Extracting to " + d_output_file_hint + ".tip and " + d_output_file_hint + ".aip");

        while (!data_in.eof())
        {
            data_in.read((char *)buffer, BUFFER_SIZE);

            int frmnum = ((buffer[6] >> 7) & 0b00000010) | ((buffer[6] >> 7) & 1);

            if (frmnum == 1 || frmnum == 3)
            {
                for (int i = 0; i < 5; i++)
                {
                    for (int j = 0; j < 104; j++)
                    {
                        frameBuffer[j] = buffer[104 * (i + 1) + j - 1] >> 2;
                    }
                    if (frmnum == 1)
                    {
                        tip_out.write((char *)&frameBuffer[0], 104);
                    }
                    else
                    {
                        aip_out.write((char *)&frameBuffer[0], 104);
                    }
                }
            }

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

        tip_out.close();
        aip_out.close();
    }

    void NOAAExtractorModule::drawUI(bool window)
    {
        ImGui::Begin(windowTitle.c_str(), NULL, window ? NULL : NOWINDOW_FLAGS);

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NOAAExtractorModule::getID()
    {
        return "noaa_extractor";
    }

    std::vector<std::string> NOAAExtractorModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> NOAAExtractorModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<NOAAExtractorModule>(input_file, output_file_hint, parameters);
    }
}