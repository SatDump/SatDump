#include "module_szesat_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"

#include "../common/scrambling.h"

namespace szesat
{
    SZESATDecoderModule::SZESATDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                  d_frame_length(parameters["frame_length"].get<int>()),
                                                                                                                                  d_thresold(parameters["thresold"].get<int>()), d_check_crc(parameters["check_crc"].get<int>()), d_enable_rs(parameters["rs_enable"].get<int>())
    {
        input_buffer = new int8_t[264];
        std::string lastReceivedPacketType = "none";
        

        deframer = std::make_unique<def::SimpleDeframer>(0x1ACFFC1F, 32, (int)((d_frame_length+4)*8), d_thresold, false, true);
    }

    SZESATDecoderModule::~SZESATDecoderModule()
    {
        delete[] input_buffer;
    }

    const uint8_t* SZESATDecoderModule::PN9_MASK_Generator(){
        uint8_t mask[8192] = {0};
        uint16_t buffer = 0x1FF;
        uint8_t xor_state = 0;
        uint8_t *mask_bytes = (uint8_t*)malloc(1024 * sizeof(uint8_t));
        memset(mask_bytes, 0, 1024);
        for(int i=0; i<8192; i++){
            mask[i] = buffer & 0b1;
            xor_state = (buffer & 0b1)^(buffer >> 5 & 0b1);
            if(xor_state){buffer = (buffer >> 1) | 0x0100;}
            else {buffer = (buffer >> 1);}
        }
        for (int i = 0; i < 1024; i++) {
            for (int j = 0; j < 8; j++) {
                mask_bytes[i] |= (mask[i * 8 + j] & 0x01) << (0 + j);
            }
        }
        return mask_bytes;
    }

    const uint8_t* SZESATDecoderModule::PN9_MASK_Generator2() {
        static uint8_t mask_bytes[1024] = { 0 };  // static so it's persistent
        uint16_t buffer = 0x1FF;
        uint8_t mask[8192] = { 0 };

        for (int i = 0; i < 8192; i++) {
            mask[i] = buffer & 0x1;
            uint8_t xor_state = (buffer & 0x01) ^ ((buffer >> 5) & 0x01);
            buffer = (buffer >> 1) | (xor_state ? 0x100 : 0x000);
        }

        for (int i = 0; i < 1024; i++) {
            for (int j = 0; j < 8; j++) {
                mask_bytes[i] |= (mask[i * 8 + j] & 0x01) << (7 - j);  // MSB-first!
            }
        }

        return mask_bytes;
    }


    std::vector<ModuleDataType> SZESATDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> SZESATDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    void SZESATDecoderModule::process()
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

        time_t lastTime = 0;
        // const uint8_t* pn9_scrambling = SZESATDecoderModule::PN9_MASK_Generator();
        const uint8_t* pn9_scrambling = SZESATDecoderModule::PN9_MASK_Generator2();
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)input_buffer, 264);
            else
                input_fifo->read((uint8_t *)input_buffer, 264);

            auto frames = deframer->work((uint8_t *)input_buffer, 264 / 8);

            for (auto &framebuf : frames)
            {
                for (int i = 0; i < (int)d_frame_length; i++)
                    framebuf[i + 4] ^= pn9_scrambling[i];

                // RS223, ccsds standard, dual-basis
                reedsolomon::ReedSolomon rs(reedsolomon::RS223);
                rs_error = -1;
                if (d_enable_rs) {
                    rs_error = rs.decode(&framebuf[4], true);
                }

                // crc
                int sizeIntoCRC = d_frame_length - 4 - 32 + 1; // -asm, -rs_array +1 virtual pad
                uint8_t frametemporary2[264] = {0};
                for (int i = 0; i < sizeIntoCRC - 1; i++) // copy
                {
                    frametemporary2[i] = framebuf[i + 4];
                }
                frametemporary2[sizeIntoCRC - 2] = 0;
                frametemporary2[sizeIntoCRC - 1] = 0;
                uint32_t crc_frm1 = crc_check.compute((uint8_t *)frametemporary2, sizeIntoCRC);
                uint32_t crc_frm2 = framebuf[4 + d_frame_length - 32 - 4] << 24 |
                    framebuf[4 + d_frame_length - 32 - 3] << 16 |
                    framebuf[4 + d_frame_length - 32 - 2] << 8 |
                    framebuf[4 + d_frame_length - 32 - 1];
                crc_ok = 0;
                if (crc_frm1 == crc_frm2)
                {
                    crc_ok = 1;
                    data_out.write((char *)framebuf.data(), framebuf.size());
                    frm_cnt++;
                    lastReceivedPacketType = SZESATDecoderModule::findMessageType(framebuf[4]);
                }
                else if (d_check_crc == false) {
                    crc_ok = -1;
                    data_out.write((char*)framebuf.data(), framebuf.size());
                    frm_cnt++;
                    lastReceivedPacketType = SZESATDecoderModule::findMessageType(framebuf[4]);
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

    void SZESATDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("SZESAT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
        {
            ImGui::Text("Frames : ");
            ImGui::SameLine();
            ImGui::TextColored(style::theme.green, UITO_C_STR(frm_cnt));


            // copied
            if (d_enable_rs) {
                ImGui::Text("RS    : ");
                if (rs_error == -1)
                    ImGui::TextColored(style::theme.red, "%i ", 1);
                else if (rs_error > 0)
                    ImGui::TextColored(style::theme.orange, "%i ", 1);
                else
                    ImGui::TextColored(style::theme.green, "%i ", 1);
            }
            if (crc_ok == -1)
                ImGui::TextColored(style::theme.blue, "CRC NOT CHECKED");
            else if (crc_ok > 0)
                ImGui::TextColored(style::theme.green, "CRC OK");
            else
                ImGui::TextColored(style::theme.red, "CRC NOT OK");
        }

        ImGui::Button("Teszt", { 200 * ui_scale, 20 * ui_scale });
        {
            ImGui::Text("Last received packet type: ");
            ImGui::SameLine();
            ImGui::TextColored(style::theme.green, "%s", lastReceivedPacketType.c_str());
        }

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string SZESATDecoderModule::findMessageType(uint8_t number) {
        switch (number) {
        case RadioMessageTypes_t::RF_PING:
            return "Ping";
        case RadioMessageTypes_t::RF_TLM_req:
            return "TLM Request";
        case RadioMessageTypes_t::RF_TinyGS_req:
            return "TinyGS Request";
        case RadioMessageTypes_t::RF_RFLSH:
            return "Read flash request";

            // etc etc etc 
        case RadioMessageTypes_t::RF_CRCPG:
            return "CRCPG request";
        case RadioMessageTypes_t::RF_CRCPG_DL:
            return "CRCPG downlink";

        case RadioMessageTypes_t::RF_RFLSH_DL:
            return "Read flash downlink";
        // 253 tinygs (lora)
        case RadioMessageTypes_t::RF_TLM_DL:
            return "TLM";
        case RadioMessageTypes_t::RF_ACK:
            return "ACK";
        default:
            return "Unknown packet type";
        }
    }

    std::string SZESATDecoderModule::getID()
    {
        return "szesat_decoder";
    }

    std::vector<std::string> SZESATDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> SZESATDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SZESATDecoderModule>(input_file, output_file_hint, parameters);
    }
}
