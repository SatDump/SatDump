#include "module_dmsp_rtd_decoder.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <volk/volk.h>

#define BUFFER_SIZE 8192
#define RTD_FRAME_SIZE 19

namespace dmsp
{
    DMSPRTDDecoderModule::DMSPRTDDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), constellation(1.0, 0.15, demod_constellation_size)
    {
        def = std::make_shared<DMSP_Deframer>(150, 2);
        soft_buffer = new int8_t[BUFFER_SIZE];
        soft_bits = new uint8_t[BUFFER_SIZE];
        output_frames = new uint8_t[BUFFER_SIZE];
        fsfsm_file_ext = ".frm";
    }

    DMSPRTDDecoderModule::~DMSPRTDDecoderModule()
    {
        delete[] soft_buffer;
        delete[] soft_bits;
        delete[] output_frames;
    }

    void DMSPRTDDecoderModule::process()
    {
        double start_timestamp = getValueOrDefault(d_parameters["start_timestamp"], -1);
        size_t accumulated_samples = 0;

        std::ofstream timestamps_out;

        if (start_timestamp != -1)
            timestamps_out = std::ofstream(d_output_file_hint + "_timestamps.txt", std::ios::binary);

        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)soft_buffer, BUFFER_SIZE);

            for (int i = 0; i < BUFFER_SIZE; i++)
                soft_bits[i] = soft_buffer[i] > 0;

            int nframes = def->work(soft_bits, BUFFER_SIZE, output_frames);

            // Count frames
            // frame_count += nframes;

            // Write to file
            if (nframes > 0)
            {
                write_data((uint8_t *)output_frames, nframes * RTD_FRAME_SIZE);

                accumulated_samples += BUFFER_SIZE;

                if (start_timestamp != -1)
                {
                    for (int i = 0; i < nframes; i++)
                    {
                        double curr_timestamp = start_timestamp + double(accumulated_samples) / 1.024e6;
                        timestamps_out << std::to_string(curr_timestamp) << '\n';
                    }
                }
            }
        }

        if (start_timestamp != -1)
            timestamps_out.close();

        logger->info("Decoding finished");

        cleanup();
    }

    nlohmann::json DMSPRTDDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        std::string deframer_state = def->getState() == def->STATE_NOSYNC ? "NOSYNC" : (def->getState() == def->STATE_SYNCING ? "SYNCING" : "SYNCED");
        v["deframer_state"] = deframer_state;
        return v;
    }

    void DMSPRTDDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("DMSP RTD Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.pushSofttAndGaussian(soft_buffer, 127, BUFFER_SIZE);
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (def->getState() == def->STATE_NOSYNC)
                    ImGui::TextColored(style::theme.red, "NOSYNC");
                else if (def->getState() == def->STATE_SYNCING)
                    ImGui::TextColored(style::theme.orange, "SYNCING");
                else
                    ImGui::TextColored(style::theme.green, "SYNCED");

                // ImGui::Text("Frames : ");

                // ImGui::SameLine();

                // ImGui::TextColored(style::theme.green, UITO_C_STR(frame_count));
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string DMSPRTDDecoderModule::getID() { return "dmsp_rtd_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> DMSPRTDDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<DMSPRTDDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace dmsp
