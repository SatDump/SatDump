#include "module_geoscan_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"

#include "../common/scrambling.h"

namespace geoscan
{
    GEOSCANDecoderModule::GEOSCANDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                  d_frame_length(parameters["frame_length"].get<int>()),
                                                                                                                                  d_thresold(parameters["thresold"].get<int>())
    {
        input_buffer = new int8_t[256];

        deframer = std::make_unique<def::SimpleDeframer>(0x930B51DE, 32, (int)((d_frame_length+4)*8), d_thresold, false, true);
    }

    GEOSCANDecoderModule::~GEOSCANDecoderModule()
    {
        delete[] input_buffer;
    }

    const uint8_t* GEOSCANDecoderModule::PN9_MASK_Generator(){
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

    std::vector<ModuleDataType> GEOSCANDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> GEOSCANDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    void GEOSCANDecoderModule::process()
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
        const uint8_t* pn9_scrambling = GEOSCANDecoderModule::PN9_MASK_Generator();
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)input_buffer, 256);
            else
                input_fifo->read((uint8_t *)input_buffer, 256);

            auto frames = deframer->work((uint8_t *)input_buffer, 256 / 8);

            for (auto &framebuf : frames)
            {
                for (int i = 0; i < (int)d_frame_length; i++)
                    framebuf[4 + i] ^= pn9_scrambling[i];

                uint16_t crc_frm1 = crc_check.compute(&framebuf[4], (int)(d_frame_length-2));
                uint16_t crc_frm2 = framebuf[4 + (int)(d_frame_length-2) + 0] << 8 | framebuf[4 + (int)(d_frame_length-2) + 1];

                if (crc_frm1 == crc_frm2)
                {
                    data_out.write((char *)framebuf.data(), framebuf.size());
                    frm_cnt++;
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

    void GEOSCANDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("GEOSCAN Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
        {
            ImGui::Text("Frames : ");
            ImGui::SameLine();
            ImGui::TextColored(style::theme.green, UITO_C_STR(frm_cnt));
        }

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string GEOSCANDecoderModule::getID()
    {
        return "geoscan_decoder";
    }

    std::vector<std::string> GEOSCANDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> GEOSCANDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<GEOSCANDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
