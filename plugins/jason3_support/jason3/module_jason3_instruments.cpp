#include "module_jason3_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_standard/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/ccsds/ccsds_standard/demuxer.h"

#include <thread>

namespace jason3
{
    namespace instruments
    {
        Jason3InstrumentsDecoderModule::Jason3InstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void Jason3InstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1279];

            // Demuxers
            ccsds::ccsds_standard::Demuxer demuxer_vcid1(1101, false, 2, 4);
            ccsds::ccsds_standard::Demuxer demuxer_vcid2(1101, false, 2, 4);

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1279);

                // Parse this transport frame
                ccsds::ccsds_standard::VCDU vcdu = ccsds::ccsds_standard::parseVCDU(cadu);

                if (vcdu.vcid == 1) // AMR-2, Poseidon
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1408) // AMR-2
                            amr2_reader.work(pkt);
                        else if (pkt.header.apid == 1031) // Poseidon-C
                            poseidon_c_reader.work(pkt);
                        else if (pkt.header.apid == 1032) // Poseidon-Ku
                            poseidon_ku_reader.work(pkt);
                    }
                }
                else if (vcdu.vcid == 2) // LPT
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid2.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1316)
                            lpt_els_a_reader.work(pkt);
                        else if (pkt.header.apid == 1317)
                            lpt_els_b_reader.work(pkt);
                        else if (pkt.header.apid == 1318)
                            lpt_aps_a_reader.work(pkt);
                        else if (pkt.header.apid == 1319)
                            lpt_aps_b_reader.work(pkt);
                    }
                }

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();

            // AMR-2
            {
                amr2_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AMR-2";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- AMR-2");
                logger->info("Lines : " + std::to_string(amr2_reader.lines));

                for (int i = 0; i < 3; i++)
                {
                    WRITE_IMAGE(amr2_reader.getChannel(i), directory + "/AMR2-" + std::to_string(i + 1));
                }
                amr2_status = DONE;
            }

            // TODO SEM & POSEIDON

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

        void Jason3InstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Jason-3 Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##jason3instrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Lines / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AMR-2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", amr2_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(amr2_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Poseidon C");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", poseidon_c_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(poseidon_c_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Poseidon Ku");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", poseidon_ku_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(poseidon_ku_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("LPT ELS-A");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", lpt_els_a_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(lpt_els_a_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("LPT ELS-B");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", lpt_els_b_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(lpt_els_b_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("LPT APS-A");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", lpt_aps_a_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(lpt_aps_a_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("LPT APS-B");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", lpt_aps_b_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(lpt_aps_b_status);

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string Jason3InstrumentsDecoderModule::getID()
        {
            return "jason3_instruments";
        }

        std::vector<std::string> Jason3InstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> Jason3InstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<Jason3InstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop