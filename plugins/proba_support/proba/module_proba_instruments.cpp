#include "module_proba_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_proba/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/image/bowtie.h"
#include "common/ccsds/ccsds_1_0_proba/demuxer.h"
#include "products/products.h"
#include "products/dataset.h"

namespace proba
{
    namespace instruments
    {
        PROBAInstrumentsDecoderModule::PROBAInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
            if (parameters["satellite"] == "proba1")
                d_satellite = PROBA_1;
            else if (parameters["satellite"] == "proba2")
                d_satellite = PROBA_2;
            else if (parameters["satellite"] == "probaV")
                d_satellite = PROBA_V;
            else
                throw std::runtime_error("Proba Instruments Decoder : Proba satellite \"" + parameters["satellite"].get<std::string>() + "\" is not valid!");
        }

        void PROBAInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1279];

            // Demuxers
            ccsds::ccsds_1_0_proba::Demuxer demuxer_vcid1(1103, false);
            ccsds::ccsds_1_0_proba::Demuxer demuxer_vcid2(1103, false);
            ccsds::ccsds_1_0_proba::Demuxer demuxer_vcid3(1103, false);

            std::ofstream output("file.ccsds");

            // Products dataset
            satdump::ProductDataSet dataset;
            if (d_satellite == PROBA_1)
                dataset.satellite_name = "PROBA-1";
            else if (d_satellite == PROBA_2)
                dataset.satellite_name = "PROBA-2";
            else if (d_satellite == PROBA_V)
                dataset.satellite_name = "PROBA-V";
            dataset.timestamp = time(0); // avg_overflowless(avhrr_reader.timestamps);

            // Init readers
            if (d_satellite == PROBA_1)
            {
                std::string chris_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CHRIS";
                std::string hrc_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/HRC";

                if (!std::filesystem::exists(chris_directory))
                    std::filesystem::create_directory(chris_directory);
                if (!std::filesystem::exists(hrc_directory))
                    std::filesystem::create_directory(hrc_directory);

                chris_reader = std::make_unique<chris::CHRISReader>(chris_directory, dataset);
                hrc_reader = std::make_unique<hrc::HRCReader>(hrc_directory);
            }
            else if (d_satellite == PROBA_2)
            {
                std::string swap_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SWAP";

                if (!std::filesystem::exists(swap_directory))
                    std::filesystem::create_directory(swap_directory);

                swap_reader = std::make_unique<swap::SWAPReader>(swap_directory);
            }

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1279);

                // Parse this transport frame
                ccsds::ccsds_1_0_proba::VCDU vcdu = ccsds::ccsds_1_0_proba::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // logger->info(vcdu.vcid);

                if (vcdu.vcid == 1) // Replay VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (d_satellite == PROBA_1)
                        {
                            if (pkt.header.apid == 0)
                            {
                                int mode_marker = pkt.payload[9] & 0x03;

                                if (mode_marker == 1 || mode_marker == 2)
                                    hrc_reader->work(pkt);
                                else
                                    chris_reader->work(pkt);
                                output.write((char *)pkt.payload.data(), 1340);
                            }
                        }
                    }
                }
                /*else if (vcdu.vcid == 2) // IDK VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid2.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        // if (d_satellite == PROBA_2)
                        //     if (pkt.header.apid == 20)
                        //         swap_reader->work(pkt);
                        logger->info("{:d}, {:d}", pkt.header.apid, pkt.payload.size());

                        if(pkt.header.apid == 22)
                        output.write((char*)pkt.payload.data(), 1340);
                    }
                }*/
                else if (vcdu.vcid == 3) // SWAP VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid3.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (d_satellite == PROBA_2)
                            if (pkt.header.apid == 20)
                                swap_reader->work(pkt);
                    }
                }

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            // CHRIS
            if (d_satellite == PROBA_1)
            {
                chris_status = SAVING;

                logger->info("----------- CHRIS");
                logger->info("Images : {:d}", chris_reader->count);

                chris_reader->save();

                chris_status = DONE;
            }

            // HRC
            if (d_satellite == PROBA_1)
            {
                hrc_status = SAVING;

                logger->info("----------- HRC");
                logger->info("Images : {:d}", hrc_reader->count);

                hrc_reader->save();

                hrc_status = DONE;
            }

            // HRC
            if (d_satellite == PROBA_2)
            {
                swap_status = SAVING;

                logger->info("----------- SWAP");
                logger->info("Images : {:d}", swap_reader->count);

                swap_reader->save();

                swap_status = DONE;
            }

            if (d_satellite == PROBA_1)
                dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void PROBAInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Proba Instruments Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##probainstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Images / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                if (d_satellite == PROBA_1)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("CHRIS");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", chris_reader->count);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(chris_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("HRC");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", hrc_reader->count);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(hrc_status);
                }

                if (d_satellite == PROBA_2)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("SWAP");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", swap_reader->count);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(swap_status);
                }

                ImGui::EndTable();
            }

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string PROBAInstrumentsDecoderModule::getID()
        {
            return "proba_instruments";
        }

        std::vector<std::string> PROBAInstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> PROBAInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<PROBAInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop