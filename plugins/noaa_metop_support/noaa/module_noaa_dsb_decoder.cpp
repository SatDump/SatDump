#include "module_noaa_dsb_decoder.h"
#include "dsb.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <volk/volk.h>

// Return filesize
uint64_t getFilesize(std::string filepath);

#define BUFFER_SIZE 8192

namespace noaa
{
    NOAADSBDecoderModule::NOAADSBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), constellation(1.0, 0.15, demod_constellation_size)
    {
        def = std::make_shared<DSB_Deframer>(DSB_FRAME_SIZE * 8, 0);
        soft_buffer = new int8_t[BUFFER_SIZE];

        fsfsm_file_ext = ".tip";
    }

    NOAADSBDecoderModule::~NOAADSBDecoderModule() { delete[] soft_buffer; }

    void NOAADSBDecoderModule::process()
    {
        uint8_t *output_pkts = new uint8_t[BUFFER_SIZE * 2];

        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)soft_buffer, BUFFER_SIZE);

            int framen = def->work(soft_buffer, BUFFER_SIZE, output_pkts);

            // Count frames
            frame_count += framen;

            // Write to file
            if (framen > 0)
            {
                for (int i = 0; i < framen; i++)
                {
                    write_data((uint8_t *)&output_pkts[i * DSB_FRAME_SIZE], DSB_FRAME_SIZE);
                }
            }
        }

        delete[] output_pkts;

        logger->info("Decoding finished");

        cleanup();
    }

    nlohmann::json NOAADSBDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["frame_count"] = frame_count;
        std::string deframer_state = def->getState() == 0 ? "NOSYNC" : (def->getState() == 2 || def->getState() == 6 ? "SYNCING" : "SYNCED");
        v["deframer_state"] = deframer_state;
        return v;
    }

    void NOAADSBDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("NOAA DSB Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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

                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(style::theme.green, UITO_C_STR(frame_count));
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string NOAADSBDecoderModule::getID() { return "noaa_dsb_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> NOAADSBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NOAADSBDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
