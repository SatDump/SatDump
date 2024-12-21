#include "module_soft2hard.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"

#include "common/codings/differential/nrzm.h"

namespace generic
{
    Soft2HardModule::Soft2HardModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        input_buffer = new int8_t[256];
    }

    Soft2HardModule::~Soft2HardModule()
    {
        delete[] input_buffer;
    }

    std::vector<ModuleDataType> Soft2HardModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> Soft2HardModule::getOutputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    void Soft2HardModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".frm");

        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        logger->info("Using input frames " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".frm");

        uint8_t byte_shifter = 0;
        int bitshift_in_byte = 0;
        uint8_t byte_buffer[256];
        int bytes_in_buf = 0;

        diff::NRZMDiff diff;

        time_t lastTime = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)input_buffer, 256);
            else
                input_fifo->read((uint8_t *)input_buffer, 256);

            for (int i = 0; i < 256; i++)
            {
                // Slice
                uint8_t bit = input_buffer[i] > 0;

                byte_shifter = byte_shifter << 1 | bit;
                bitshift_in_byte++;

                if (bitshift_in_byte == 8)
                {
                    byte_buffer[bytes_in_buf] = byte_shifter;
                    bitshift_in_byte = 0;
                    bytes_in_buf++;

                    if (bytes_in_buf == 256)
                    {
                        // diff.decode(byte_buffer, 256);
                        if (output_data_type == DATA_FILE)
                            data_out.write((char *)byte_buffer, 256);
                        else
                            output_fifo->write((uint8_t *)byte_buffer, 256);

                        bytes_in_buf = 0;
                    }
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void Soft2HardModule::drawUI(bool window)
    {
        ImGui::Begin("Soft To Hard", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string Soft2HardModule::getID()
    {
        return "soft2hard";
    }

    std::vector<std::string> Soft2HardModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> Soft2HardModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<Soft2HardModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
