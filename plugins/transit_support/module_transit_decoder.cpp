#include "module_transit_decoder.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <volk/volk.h>

// Return filesize
uint64_t getFilesize(std::string filepath);

#define BUFFER_SIZE 8192

namespace transit
{
    TransitDecoderModule::TransitDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), constellation(1.0, 0.15, demod_constellation_size)
    {;
        def = std::make_shared<TransitDeframer>(0);

        soft_buffer = new int8_t[BUFFER_SIZE];

        fsfsm_file_ext = ".frm";
    }

    TransitDecoderModule::~TransitDecoderModule() { delete[] soft_buffer; }

    void TransitDecoderModule::process()
    {
        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)soft_buffer, BUFFER_SIZE);

            std::vector<std::vector<uint8_t>> frames = def->work(soft_buffer, BUFFER_SIZE/8);

            // Count frames
            int framen = frames.size();
            frame_count += framen;

            // Write to file
            if (framen > 0)
            {
                for (int i = 0; i < framen; i++)
                {
                    // get frame counter
                    uint16_t fc = (frames[i][16] << 8) | frames[i][17];
                    uint32_t fc_fake = fc << 1;
                    uint8_t  subframe = frames[i][1] & 1;
                    fc_fake |= subframe;

                    if(fc_fake == last_framecounter + 1)
                    {
                        coherent_frames++;
                        framecounter_coherent = true;
                    }
                    else
                    {
                        framecounter_coherent = false;
                    }

                    last_framecounter = fc_fake;

                    write_data((uint8_t *)frames[i].data(), 32);
                }
            }
        }

        logger->info("Decoding finished");

        cleanup();
    }

    nlohmann::json TransitDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["frame_count"] = frame_count;
        v["frame_count_coherent"] = coherent_frames;

        std::string deframer_state = def->getState() == 1 ? "SYNCED" : "NOSYNC";
        v["deframer_state"] = deframer_state;
        
        std::string satellite_state;
        if(def->getState() == 1)
        {
            if (framecounter_coherent)
                satellite_state = "Coherent";
            else
                satellite_state = "Incoherent";
        }
        else
        {
            satellite_state = "N/A";
        }
        v["satellite_state"] = satellite_state;
        return v;
    }

    void TransitDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Transit Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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

                if (def->getState() == 1)
                    ImGui::TextColored(style::theme.green, "SYNCED");
                else
                    ImGui::TextColored(style::theme.red, "NOSYNC");

                ImGui::Text("Sat State : ");

                ImGui::SameLine();

                if(def->getState() == 1)
                {
                    if (framecounter_coherent)
                        ImGui::TextColored(style::theme.green, "Coherent");
                    else
                        ImGui::TextColored(style::theme.red, "Incoherent");
                }
                else
                {
                    ImGui::TextColored(style::theme.red, "N/A");
                }

                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(style::theme.green, UITO_C_STR(frame_count));

                ImGui::Text("Coherent Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(style::theme.green, UITO_C_STR(coherent_frames));
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string TransitDecoderModule::getID() { return "transit_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> TransitDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<TransitDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
