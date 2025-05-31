#include "module_ax25_decoder.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"

#include "common/codings/deframing/hdlc_def.h"
#include "common/codings/differential/nrzi.h"
#include "common/codings/lfsr.h"
#include <cstdint>

namespace ax25
{
    AX25DecoderModule::AX25DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        input_buffer = new int8_t[256];

        d_nrzi = parameters.contains("nrzi") ? parameters["nrzi"].get<bool>() : false;
        d_g3ruh = parameters.contains("g3ruh") ? parameters["g3ruh"].get<bool>() : false;

        fsfsm_file_ext = ".frm";
    }

    AX25DecoderModule::~AX25DecoderModule() { delete[] input_buffer; }

    void AX25DecoderModule::process()
    {
        diff::NRZIDiff nrzi_decoder;
        common::lfsr g3rhu_descramble(0x21, 0x0, 16);
        HDLCDeframer hdlc_deframer(d_parameters["min_sz"].get<int>(), d_parameters["max_sz"].get<int>());

        uint8_t bits_buf[256];

        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)input_buffer, 256);

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

                logger->trace("AX.25 Frame - SRC : " + source_callsign + ",%d DST : " + destination_callsign + ",%d CTRL : %d PID : %d", source_ssid, target_ssid, ctrl, pid);

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
                write_data((uint8_t *)frm.data(), frm.size());

                frm_cnt++;
            }
        }

        logger->info("Decoding finished");

        cleanup();
    }

    nlohmann::json AX25DecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["frm_cnt"] = frm_cnt;
        return v;
    }

    void AX25DecoderModule::drawUI(bool window)
    {
        ImGui::Begin("AX-25 Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
        {
            ImGui::Text("Frames : ");
            ImGui::SameLine();
            ImGui::TextColored(style::theme.green, UITO_C_STR(frm_cnt));
        }

        drawProgressBar();

        ImGui::End();
    }

    std::string AX25DecoderModule::getID() { return "ax25_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> AX25DecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<AX25DecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace ax25
