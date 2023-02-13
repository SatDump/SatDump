#include "module_lucky7_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/repack_bits_byte.h"
#include "common/scrambling.h"
#include "common/codings/crc/crc_generic.h"

#define BUFFER_SIZE 100
#define FRAME_SIZE_FULL 312
#define FRAME_SIZE_PAYLOAD 280

namespace lucky7
{
    Lucky7DecoderModule::Lucky7DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                constellation(1.0, 0.15, demod_constellation_size)
    {
        soft_buffer = new int8_t[BUFFER_SIZE];
        byte_buffers = new uint8_t[BUFFER_SIZE];
    }

    std::vector<ModuleDataType> Lucky7DecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> Lucky7DecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    Lucky7DecoderModule::~Lucky7DecoderModule()
    {
        delete[] soft_buffer;
        delete[] byte_buffers;
    }

    void Lucky7DecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".frm");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".frm");

        RepackBitsByte repacker;
        def::SimpleDeframer deframer(0b0010110111010100, 16, FRAME_SIZE_FULL * 8, 0);
        codings::crc::GenericCRC crc_check(16, 0x8005, 0xFFFF, 0x0000, false, false);

        time_t lastTime = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)soft_buffer, BUFFER_SIZE);

            for (int i = 0; i < BUFFER_SIZE; i++)
                soft_buffer[i] = soft_buffer[i] > 0;

            int b = repacker.work((uint8_t *)soft_buffer, BUFFER_SIZE, byte_buffers);

            auto frames = deframer.work(byte_buffers, b);

            for (auto &frm : frames)
            {
                cubesat::scrambling::si4462_scrambling(&frm[2], FRAME_SIZE_PAYLOAD);

                uint16_t crc_frm1 = crc_check.compute(&frm[2], 35);
                uint16_t crc_frm2 = frm[2 + 35 + 0] << 8 | frm[2 + 35 + 1];

                if (crc_frm1 == crc_frm2)
                {
                    data_out.write((char *)&frm[2], 35);
                    frm_cnt++;
                }
                else
                {
                    logger->error("Invalid CRC!");
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Frames : " + std::to_string(frm_cnt));
            }
        }

        logger->info("Decoding finished");

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void Lucky7DecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Lucky-7 Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.pushSofttAndGaussian(soft_buffer, 127, BUFFER_SIZE);
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), UITO_C_STR(frm_cnt));
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string Lucky7DecoderModule::getID()
    {
        return "lucky7_decoder";
    }

    std::vector<std::string> Lucky7DecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> Lucky7DecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<Lucky7DecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
