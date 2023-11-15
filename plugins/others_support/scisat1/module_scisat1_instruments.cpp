#include "module_scisat1_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_standard/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/image/bowtie.h"
#include "common/ccsds/ccsds_standard/demuxer.h"
#include "products/products.h"
#include "products/dataset.h"

namespace scisat1
{
    namespace instruments
    {
        SciSat1InstrumentsDecoderModule::SciSat1InstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void SciSat1InstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1199];

            // Demuxers
            ccsds::ccsds_standard::Demuxer demuxer_vcid0(1016, false, 2, 7);
            ccsds::ccsds_standard::Demuxer demuxer_vcid1(1016, false, 2, 7);
            ccsds::ccsds_standard::Demuxer demuxer_vcid4(1016, false, 2, 7);

            // std::ofstream output("file.ccsds");

            // Products dataset
            // satdump::ProductDataSet dataset;
            // dataset.satellite_name = "PROBA-1";
            // dataset.timestamp = time(0); // avg_overflowless(avhrr_reader.timestamps);

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1199);

                // Parse this transport frame
                ccsds::ccsds_standard::VCDU vcdu = ccsds::ccsds_standard::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // logger->info(vcdu.vcid);

                if (vcdu.vcid == 0) // FTS
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 101) // FTS I/Q Samples
                            fts_reader.work(pkt);
                }
                else if (vcdu.vcid == 1) // MAESTRO? Doesn't seem to be VNIRI spec-wise
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid4.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 102) // MAESTRO Spectrometer data? Doesn't seem to be VNIRI spec-wise
                            maestro_reader.work(pkt);
                }
                else if (vcdu.vcid == 4) // Mainly HK it seems like
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid4.work(cadu);
                    // for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    //{
                    // }
                }

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();

            // FTS
            {
                fts_status = SAVING;

                logger->info("----------- FTS");
                logger->info("Lines : %d", fts_reader.lines);

                std::string fts_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/FTS";

                if (!std::filesystem::exists(fts_directory))
                    std::filesystem::create_directory(fts_directory);

                fts_reader.getImg().save_img(fts_directory + "/FTS");

                auto img = fts_reader.getImg();
                img.resize_bilinear(img.width() / 10, img.height() * 10);
                img.save_img(fts_directory + "/FTS_scaled");

                fts_status = DONE;
            }

            // MAESTRO
            {
                maestro_status = SAVING;

                logger->info("----------- MAESTRO");
                logger->info("Lines (1) : %d", maestro_reader.lines_1);
                logger->info("Lines (2) : %d", maestro_reader.lines_2);

                std::string maestro_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MAESTRO";

                if (!std::filesystem::exists(maestro_directory))
                    std::filesystem::create_directory(maestro_directory);

                maestro_reader.getImg1().save_img(maestro_directory + "/MAESTRO_1");
                maestro_reader.getImg2().save_img(maestro_directory + "/MAESTRO_2");

                maestro_status = DONE;
            }
        }

        void SciSat1InstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("SciSat-1 Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##scisat1instrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
                ImGui::Text("FTS");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", fts_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(fts_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MAESTRO Mode 1");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", maestro_reader.lines_1);
                ImGui::TableSetColumnIndex(2);
                drawStatus(maestro_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MAESTRO Mode 2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", maestro_reader.lines_2);
                ImGui::TableSetColumnIndex(2);
                drawStatus(maestro_status);

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string SciSat1InstrumentsDecoderModule::getID()
        {
            return "scisat1_instruments";
        }

        std::vector<std::string> SciSat1InstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> SciSat1InstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<SciSat1InstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop