#include "module_noaa_hrpt_decoder.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <volk/volk.h>

// Return filesize
uint64_t getFilesize(std::string filepath);

#define BUFSIZE 8192

namespace noaa
{
    NOAAHRPTDecoderModule::NOAAHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), constellation(1.0, 0.15, demod_constellation_size)
    {
        // Buffers
        soft_buffer = new int8_t[BUFSIZE];
        def = std::make_shared<NOAADeframer>(d_parameters["deframer_thresold"].get<int>());
        fsfsm_file_ext = ".raw16";
    }

    NOAAHRPTDecoderModule::~NOAAHRPTDecoderModule() { delete[] soft_buffer; }

    void NOAAHRPTDecoderModule::process()
    {
        while (should_run())
        {
            read_data((uint8_t *)soft_buffer, BUFSIZE);

            std::vector<uint16_t> frames = def->work(soft_buffer, BUFSIZE);

            // Count frames
            frame_count += frames.size();

            // Write to file
            if (frames.size() > 0)
                write_data((uint8_t *)&frames[0], frames.size() * sizeof(uint16_t));
        }

        logger->info("Demodulation finished");

        cleanup();
    }

    nlohmann::json NOAAHRPTDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["frame_count"] = frame_count / 11090;
        return v;
    }

    void NOAAHRPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("NOAA HRPT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        // Constellation
        ImGui::BeginGroup();
        constellation.pushSofttAndGaussian(soft_buffer, 127, BUFSIZE);
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(style::theme.green, UITO_C_STR(frame_count / 11090));
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string NOAAHRPTDecoderModule::getID() { return "noaa_hrpt_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> NOAAHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NOAAHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
