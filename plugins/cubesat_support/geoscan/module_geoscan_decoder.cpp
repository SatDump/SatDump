#include "module_geoscan_decoder.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"

#include "../common/scrambling.h"
#include <cstdint>

namespace geoscan
{
    GEOSCANDecoderModule::GEOSCANDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), d_frame_length(parameters["frame_length"].get<int>()),
          d_thresold(parameters["thresold"].get<int>())
    {
        input_buffer = new int8_t[256];

        deframer = std::make_unique<def::SimpleDeframer>(0x930B51DE, 32, (int)((d_frame_length + 4) * 8), d_thresold, false, true);

        fsfsm_file_ext = ".frm";
    }

    GEOSCANDecoderModule::~GEOSCANDecoderModule() { delete[] input_buffer; }

    const uint8_t *GEOSCANDecoderModule::PN9_MASK_Generator()
    {
        uint8_t mask[8192] = {0};
        uint16_t buffer = 0x1FF;
        uint8_t xor_state = 0;
        uint8_t *mask_bytes = (uint8_t *)malloc(1024 * sizeof(uint8_t));
        memset(mask_bytes, 0, 1024);
        for (int i = 0; i < 8192; i++)
        {
            mask[i] = buffer & 0b1;
            xor_state = (buffer & 0b1) ^ (buffer >> 5 & 0b1);
            if (xor_state)
            {
                buffer = (buffer >> 1) | 0x0100;
            }
            else
            {
                buffer = (buffer >> 1);
            }
        }
        for (int i = 0; i < 1024; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                mask_bytes[i] |= (mask[i * 8 + j] & 0x01) << (0 + j);
            }
        }
        return mask_bytes;
    }

    void GEOSCANDecoderModule::process()
    {
        const uint8_t *pn9_scrambling = GEOSCANDecoderModule::PN9_MASK_Generator();
        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)input_buffer, 256);

            auto frames = deframer->work((uint8_t *)input_buffer, 256 / 8);

            for (auto &framebuf : frames)
            {
                for (int i = 0; i < (int)d_frame_length; i++)
                    framebuf[4 + i] ^= pn9_scrambling[i];

                uint16_t crc_frm1 = crc_check.compute(&framebuf[4], (int)(d_frame_length - 2));
                uint16_t crc_frm2 = framebuf[4 + (int)(d_frame_length - 2) + 0] << 8 | framebuf[4 + (int)(d_frame_length - 2) + 1];

                if (crc_frm1 == crc_frm2)
                {
                    write_data((uint8_t *)framebuf.data(), framebuf.size());
                    frm_cnt++;
                }
            }
        }

        logger->info("Decoding finished");

        cleanup();
    }

    nlohmann::json GEOSCANDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["frm_cnt"] = frm_cnt;
        return v;
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

        drawProgressBar();

        ImGui::End();
    }

    std::string GEOSCANDecoderModule::getID() { return "geoscan_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> GEOSCANDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<GEOSCANDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace geoscan
