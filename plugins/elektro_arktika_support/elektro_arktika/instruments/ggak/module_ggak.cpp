#include "module_ggak.h"
#include "imgui/imgui.h"
#include "logger.h"

namespace elektro_arktika
{
    namespace ggak
    {
        GGAKDecoderModule::GGAKDecoderModule(std::string input_file,
                                             std::string output_file_hint,
                                             nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void GGAKDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("ELEKTRO / ARKTIKA GGAK Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##ggaktable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Frames");
                ImGui::TableSetColumnIndex(2); 
                ImGui::Text("Status");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("FM-VE Magnetometer");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", ui_mag_count.load(std::memory_order_relaxed));
                ImGui::TableSetColumnIndex(2);
                drawStatus(inst_status[STATUS_MAG]);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("GALS-VE + SKIF-VE MIP");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", ui_particle_count.load(std::memory_order_relaxed));
                ImGui::TableSetColumnIndex(2);
                drawStatus(inst_status[STATUS_PARTICLES]);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("SKIF-VE ESA");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d",
                    ui_subpackets_20_count.load(std::memory_order_relaxed) +
                    ui_subpackets_30_count.load(std::memory_order_relaxed));
                ImGui::TableSetColumnIndex(2);
                drawStatus(inst_status[STATUS_SKIF_ESA]);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("SKL-E Spectral");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", ui_subpackets_5c_count.load(std::memory_order_relaxed));
                ImGui::TableSetColumnIndex(2);
                drawStatus(inst_status[STATUS_SKL]);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Housekeeping");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d",
                    ui_subpackets_00_count.load(std::memory_order_relaxed) +
                    ui_subpackets_10_count.load(std::memory_order_relaxed));
                ImGui::TableSetColumnIndex(2);
                drawStatus(inst_status[STATUS_HK]);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (profile_detected)
                    ImGui::Text("%s", sat_profile.generation);
                else
                    ImGui::Text("Telemetry Mux");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d",
                    ui_total_frames.load(std::memory_order_relaxed) -
                    ui_fill_frames.load(std::memory_order_relaxed));
                ImGui::TableSetColumnIndex(2);
                drawStatus(inst_status[STATUS_BND]);

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string GGAKDecoderModule::getID()
        {
            return "elektro_arktika_ggak";
        }

        std::shared_ptr<satdump::pipeline::ProcessingModule>
        GGAKDecoderModule::getInstance(std::string input_file,
                                       std::string output_file_hint,
                                       nlohmann::json parameters)
        {
            return std::make_shared<GGAKDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace ggak
} // namespace elektro_arktika
