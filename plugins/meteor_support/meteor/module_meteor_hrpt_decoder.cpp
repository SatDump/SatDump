#include "module_meteor_hrpt_decoder.h"
#include "common/codings/manchester.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>

#define BUFFER_SIZE 8192

namespace meteor
{
    METEORHRPTDecoderModule::METEORHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), constellation(1.0, 0.15, demod_constellation_size)
    {
        def = std::make_shared<CADUDeframer>();

        soft_buffer = new int8_t[BUFFER_SIZE];

        fsfsm_file_ext = ".cadu";
    }

    METEORHRPTDecoderModule::~METEORHRPTDecoderModule() { delete[] soft_buffer; }

    void METEORHRPTDecoderModule::process()
    {
        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)soft_buffer, BUFFER_SIZE);

            std::vector<std::array<uint8_t, CADU_SIZE>> frames = def->work(soft_buffer, BUFFER_SIZE);

            // Count frames
            frame_count += frames.size();

            // Write to file
            if (frames.size() > 0)
            {
                for (std::array<uint8_t, CADU_SIZE> frm : frames)
                {
                    write_data(&frm[0], CADU_SIZE);
                }
            }
        }

        logger->info("Decoding finished");

        cleanup();
    }

    nlohmann::json METEORHRPTDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["deframer_lock"] = def->getState() == 12;
        std::string deframer_state = def->getState() == 0 ? "NOSYNC" : (def->getState() == 2 || def->getState() == 6 ? "SYNCING" : "SYNCED");
        v["deframer_state"] = deframer_state;
        v["frame_count"] = frame_count;
        return v;
    }

    void METEORHRPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("METEOR HRPT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        // Constellation
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

                if (def->getState() == 0)
                    ImGui::TextColored(style::theme.red, "NOSYNC");
                else if (def->getState() == 2 || def->getState() == 6)
                    ImGui::TextColored(style::theme.orange, "SYNCING");
                else
                    ImGui::TextColored(style::theme.green, "SYNCED");
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string METEORHRPTDecoderModule::getID() { return "meteor_hrpt_decoder"; }

    std::shared_ptr<ProcessingModule> METEORHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<METEORHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor