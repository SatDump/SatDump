#include "module_transit_data.h"
#include "common/repack.h"
#include "common/tracking/tle.h"
#include "common/utils.h"
#include "core/resources.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "products/dataset.h"
#include "products/punctiform_product.h"
#include "utils/stats.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace transit
{
    TransitDataDecoderModule::TransitDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        fsfsm_enable_output = false;
    }

    void TransitDataDecoderModule::process()
    {
        uint8_t buffer[32] = {0};

        uint16_t subcom[21];

        // This is bad, C++ify it instead
        std::string tlm_csv_filename = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/tlm.csv";
        debug_f = fopen(tlm_csv_filename.c_str(), "w+");

        while (should_run())
        {
            read_data((uint8_t *)buffer, 32);

            // get FC
            uint16_t fc = (buffer[16] << 8) | buffer[17];
            uint32_t fc_fake = fc << 1; // make frame counter for checking coherence
            int subframe = buffer[1] & 1;
            fc_fake |= subframe;

            // last frame was coherent
            // this process will always miss the last frame
            if(fc_fake == last_fc + 1)
            {
                // process last frame

                // Assuming the telemetry follows that of 1963 38c, The satellite telemetry frame should contain 14 16 bit data registers, 1 16 bit sync word, and 1 16 bit frame counter.
                // a full frame is composed of 2 subframes, both subframes have the same value in the frame counter and are distinguished by the sync word
                // 12 of the 14 data registers are used for detector data (likely broken) and one of the remaining register contains telltales and and a digitised version of the PAM telemetry
                // I don't know whether any of this is still functional, given the look of the frames it's probably mostly not, but there are clear repeating patterns in the words

                // if(subframe == 0)
                {
                    uint16_t words[16];

                    for(int word=0; word < 16; word++)
                    {
                        words[word] = buffer[word*2] << 8 | buffer[word*2 + 1];
                        fprintf(debug_f, "%u,", words[word]);
                    }

                    // temp1 = words[13];
                    // subcom[fc % 21] = temp1;

                    // if(!(fc % 21))
                    // {
                    //     for(int i=0; i < 21; i++)
                    //     {
                    //         fprintf(debug_f, "%u,", subcom[i]);
                    //     }
                    //     fprintf(debug_f, "\n");
                    // }

                    fprintf(debug_f, "\n");
                }
            }
    
            last_fc = fc_fake;
            memcpy(last_frame, buffer, 32);
        }

        // done with the frames
        cleanup();

        int norad = 965;
        std::string sat_name = "Transit 5B-5";

        // Products dataset
        // satdump::products::DataSet dataset;
        // dataset.satellite_name = sat_name;
        // dataset.timestamp = ?;

        // std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry->get_from_norad_time(norad, dataset.timestamp);

        // dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));

        fclose(debug_f);
    }

    void TransitDataDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Transit Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (ImGui::BeginTable("##transitinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Instrument");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Lines / Frames");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("Status");

            ImGui::EndTable();
        }

        drawProgressBar();

        ImGui::End();
    }

    std::string TransitDataDecoderModule::getID() { return "transit_data"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> TransitDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<TransitDataDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace transit
