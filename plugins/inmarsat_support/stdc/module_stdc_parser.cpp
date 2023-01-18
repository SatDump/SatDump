#include "module_stdc_parser.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"

namespace inmarsat
{
    namespace stdc
    {
        STDCParserModule::STDCParserModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
            buffer = new uint8_t[FRAME_SIZE_BYTES];
        }

        std::vector<ModuleDataType> STDCParserModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> STDCParserModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        STDCParserModule::~STDCParserModule()
        {
            delete[] buffer;
        }

        void STDCParserModule::process()
        {
            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)buffer, FRAME_SIZE_BYTES);
                else
                    input_fifo->read((uint8_t *)buffer, FRAME_SIZE_BYTES);

                // WIP

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            if (input_data_type == DATA_FILE)
                data_in.close();
        }

        void STDCParserModule::drawUI(bool window)
        {
            ImGui::Begin("Inmarsat STD-C Parser", NULL, window ? 0 : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string STDCParserModule::getID()
        {
            return "inmarsat_stdc_parser";
        }

        std::vector<std::string> STDCParserModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> STDCParserModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<STDCParserModule>(input_file, output_file_hint, parameters);
        }
    }
}