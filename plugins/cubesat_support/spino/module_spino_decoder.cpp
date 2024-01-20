#include "module_spino_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/image/image.h"
#include "common/codings/crc/crc_generic.h"

#define BUFFER_SIZE 256

namespace spino
{
    SpinoDecoderModule::SpinoDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        input_buffer = new int8_t[BUFFER_SIZE];
    }

    SpinoDecoderModule::~SpinoDecoderModule()
    {
        delete[] input_buffer;
    }

    std::vector<ModuleDataType> SpinoDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> SpinoDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    void SpinoDecoderModule::process()
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

        uint8_t bits_buf[BUFFER_SIZE];

        // Deframer
        const uint32_t d_syncword = 0x2EFC9827;
        uint32_t def_shifter = 0;
        bool def_in_frame = false;
        int def_bitn = 0;
        uint8_t def_bytshift;
        std::vector<uint8_t> wip_frame;

        codings::crc::GenericCRC crc_ccitt(16, 0x1021, 0x0000, 0x0000, false, false);

        time_t lastTime = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)input_buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)input_buffer, BUFFER_SIZE);

            // Slice
            for (int i = 0; i < BUFFER_SIZE; i++)
                bits_buf[i] = input_buffer[i] > 0;

            // Deframe

            for (int ibit = 0; ibit < BUFFER_SIZE; ibit++)
            {
                uint8_t bit = bits_buf[ibit];
                def_shifter = def_shifter << 1 | bit;

                if (def_in_frame)
                {
                    def_bytshift = def_bytshift << 1 | bit;
                    def_bitn++;

                    if (def_bitn == 8)
                    {
                        wip_frame.push_back(def_bytshift);
                        def_bitn = 0;
                    }

                    // Here we should have a full Spino header
                    if (wip_frame.size() >= 18)
                    {
                        int size = wip_frame[17] << 8 | wip_frame[16];

                        if (wip_frame.size() >= size + 16)
                        {
                            // CRC Check
                            uint64_t crc = wip_frame[wip_frame.size() - 1] << 8 | wip_frame[wip_frame.size() - 2];
                            if (crc == crc_ccitt.compute(wip_frame.data(), wip_frame.size() - 2))
                            {
                                uint16_t pkt_sz = wip_frame.size();
                                uint8_t buffer_sz[6];
                                buffer_sz[0] = 0x1a;
                                buffer_sz[1] = 0xcf;
                                buffer_sz[2] = 0xfc;
                                buffer_sz[3] = 0x1d;
                                buffer_sz[4] = pkt_sz >> 8;
                                buffer_sz[5] = pkt_sz & 0xFF;

                                wip_frame.insert(wip_frame.begin(), buffer_sz, buffer_sz + 6);
                                data_out.write((char *)wip_frame.data(), wip_frame.size());
                                logger->info(size);
                                frm_cnt++;
                            }

                            def_in_frame = false;
                        }
                    }

                    continue;
                }

                if (def_shifter == d_syncword)
                {
                    def_in_frame = true;
                    def_bitn = 0;
                    wip_frame.clear();
                }
            }

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, Frames " + std::to_string(frm_cnt));
            }
        }

        logger->info("Decoding finished");

        data_in.close();
    }

    void SpinoDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Spino Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
        {
            ImGui::Text("Frames : ");
            ImGui::SameLine();
            ImGui::TextColored(IMCOLOR_GREEN, UITO_C_STR(frm_cnt));
        }

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string SpinoDecoderModule::getID()
    {
        return "spino_decoder";
    }

    std::vector<std::string> SpinoDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> SpinoDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SpinoDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
