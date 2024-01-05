#include "module_aim_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_standard/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/ccsds/ccsds_standard/demuxer.h"
#include "products/products.h"
#include "products/dataset.h"

namespace aim
{
    namespace instruments
    {
        AIMInstrumentsDecoderModule::AIMInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void AIMInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1244];

            // Demuxers
            ccsds::ccsds_standard::Demuxer demuxer_vcid1(1062, true, 12);

            // std::ofstream output("file.ccsds");

            // Products dataset
            // satdump::ProductDataSet dataset;
            // dataset.satellite_name = "AIM";
            // dataset.timestamp = time(0); // avg_overflowless(avhrr_reader.timestamps);

            cips_readers[0].init(340, 170);
            cips_readers[1].init(170, 340);
            cips_readers[2].init(340, 170);
            cips_readers[3].init(170, 340);

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1244);

                // Parse this transport frame
                ccsds::ccsds_standard::VCDU vcdu = ccsds::ccsds_standard::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // logger->info(vcdu.vcid);

                if (vcdu.vcid == 1) // Replay VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 360)
                            cips_readers[0].work(pkt);
                        else if (pkt.header.apid == 361)
                            cips_readers[1].work(pkt);
                        else if (pkt.header.apid == 362)
                            cips_readers[2].work(pkt);
                        else if (pkt.header.apid == 363)
                            cips_readers[3].work(pkt);
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

            // CIPS
            {
                for (int i = 0; i < 4; i++)
                {
                    cips_status[i] = SAVING;

                    logger->info("----------- CIPS %d", i + 1);
                    logger->info("Images : %d", cips_readers[i].images.size());

                    std::string cips_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CIPS-" + std::to_string(i + 1);

                    if (!std::filesystem::exists(cips_directory))
                        std::filesystem::create_directory(cips_directory);

                    int img_cnt = 0;
                    for (auto &img : cips_readers[i].images)
                    {
                        img.save_img(cips_directory + "/CIPS_" + std::to_string(img_cnt++ + 1));
                    }

                    cips_status[i] = DONE;
                }
            }

            // dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void AIMInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("AIM Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##aiminstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Images / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                for (int i = 0; i < 4; i++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("CIPS %d", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", (int)cips_readers[i].images.size());
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(cips_status[i]);
                }

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string AIMInstrumentsDecoderModule::getID()
        {
            return "aim_instruments";
        }

        std::vector<std::string> AIMInstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> AIMInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<AIMInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop