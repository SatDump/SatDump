#include "module_ax25_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/image/image.h"

#include "common/codings/differential/nrzi.h"
#include "common/codings/lfsr.h"
#include "common/codings/deframing/hdlc_def.h"

namespace ax25
{
    AX25DecoderModule::AX25DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        input_buffer = new int8_t[256];

        d_nrzi = parameters.contains("nrzi") ? parameters["nrzi"].get<bool>() : false;
        d_g3ruh = parameters.contains("g3ruh") ? parameters["g3ruh"].get<bool>() : false;
    }

    AX25DecoderModule::~AX25DecoderModule()
    {
        delete[] input_buffer;
    }

    std::vector<ModuleDataType> AX25DecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> AX25DecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    void AX25DecoderModule::process()
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
        logger->info("Decoding to " + directory);

        diff::NRZIDiff nrzi_decoder;
        common::lfsr g3rhu_descramble(0x21, 0x0, 16);
        HDLCDeframer hdlc_deframer(d_parameters["min_sz"].get<int>(), d_parameters["max_sz"].get<int>());

        uint8_t bits_buf[256];

        time_t lastTime = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)input_buffer, 256);
            else
                input_fifo->read((uint8_t *)input_buffer, 256);

            // Slice
            for (int i = 0; i < 256; i++)
                bits_buf[i] = input_buffer[i] > 0;

            // Optional NRZ-I
            if (d_nrzi)
                nrzi_decoder.decode_bits(bits_buf, 256);

            // Optional G3RUH
            for (int i = 0; i < 256; i++)
                bits_buf[i] = g3rhu_descramble.next_bit_descramble(bits_buf[i]);

            // Deframe
            auto frames = hdlc_deframer.work(bits_buf, 256);

            for (auto frm : frames)
            {
                std::string source_callsign;
                for (int i = 0; i < 6; i++)
                    source_callsign += frm[i] >> 1;

                std::string destination_callsign;
                for (int i = 7; i < 7 + 6; i++)
                    destination_callsign += frm[i] >> 1;

                int source_ssid = frm[6] >> 1;
                int target_ssid = frm[7 + 6] >> 1;

                int ctrl = frm[14];
                int pid = frm[15];

                logger->trace("AX.25 Frame - SRC : " + source_callsign + ",%d DST : " + destination_callsign + ",%d CTRL : %d PID : %d",
                              source_ssid, target_ssid, ctrl, pid);

                // Add a custom header to ease up forwarding to the next stuff
                uint16_t pkt_sz = frm.size();

                uint8_t buffer_sz[6];
                buffer_sz[0] = 0x1a;
                buffer_sz[1] = 0xcf;
                buffer_sz[2] = 0xfc;
                buffer_sz[3] = 0x1d;
                buffer_sz[4] = pkt_sz >> 8;
                buffer_sz[5] = pkt_sz & 0xFF;

                frm.insert(frm.begin(), buffer_sz, buffer_sz + 6);
                data_out.write((char *)frm.data(), frm.size());

                frm_cnt++;
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

    void AX25DecoderModule::drawUI(bool window)
    {
        ImGui::Begin("AX-25 Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
        {
            ImGui::Text("Frames : ");
            ImGui::SameLine();
            ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), UITO_C_STR(frm_cnt));
        }

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string AX25DecoderModule::getID()
    {
        return "ax25_decoder";
    }

    std::vector<std::string> AX25DecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> AX25DecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<AX25DecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
