#include "module_proba_instruments.h"
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
            ccsds::ccsds_standard::Demuxer demuxer_vcid1(1103, false);
            ccsds::ccsds_standard::Demuxer demuxer_vcid2(1103, false);
            ccsds::ccsds_standard::Demuxer demuxer_vcid3(1103, false);
            ccsds::ccsds_standard::Demuxer demuxer_vcid4(1103, false);

            // std::ofstream output("file.ccsds");

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
            else if (d_satellite == PROBA_V)
            {
                std::string vegs_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Vegetation";

                if (!std::filesystem::exists(vegs_directory))
                    std::filesystem::create_directory(vegs_directory);

                for (int i = 0; i < 3; i++)
                {
                    vegs_readers[i][0] = std::make_unique<vegetation::VegetationS>(7818, 5200);
                    vegs_readers[i][1] = std::make_unique<vegetation::VegetationS>(7818, 5200);
                    vegs_readers[i][2] = std::make_unique<vegetation::VegetationS>(7818, 5200);
                    vegs_readers[i][3] = std::make_unique<vegetation::VegetationS>(1554, 1024);
                    vegs_readers[i][4] = std::make_unique<vegetation::VegetationS>(1554, 1024);
                    vegs_readers[i][5] = std::make_unique<vegetation::VegetationS>(1554, 1024);

                    for (int y = 0; y < 6; y++)
                        vegs_status[i][y] = DECODING;
                }

                gps_ascii_reader = std::make_unique<gps_ascii::GPSASCII>(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/gps_logs.txt");
            }

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1279);

                // Parse this transport frame
                ccsds::ccsds_standard::VCDU vcdu = ccsds::ccsds_standard::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // logger->info(vcdu.vcid);
                // printf("VCID %d\n", vcdu.vcid);

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
                                // pkt.payload.resize(16000);
                                // output.write((char *)pkt.payload.data(), 16000);
                                if (mode_marker == 1 || mode_marker == 2)
                                    hrc_reader->work(pkt);
                                else
                                    chris_reader->work(pkt);
                            }
                        }
                        else if (d_satellite == PROBA_V)
                        {
                            if (pkt.header.apid == 18) // GPS ASCII Logs
                                gps_ascii_reader->work(pkt);
                        }
                    }
                }
                else if (vcdu.vcid == 2) // IDK VCID
                {
#if 0
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid2.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (d_satellite == PROBA_V)
                        {
                            printf("APID %d\n", pkt.header.apid);

                            if (pkt.header.apid == 7)
                            {
                                pkt.payload.resize(16000);
                                output.write((char *)pkt.payload.data(), 16000);
                            }
                        }
                        // if (pkt.header.apid != 2047)
                        //    logger->info("%d, %d", pkt.header.apid, pkt.payload.size());

                        // if (pkt.header.apid == 103)
                        // {
                        //     pkt.payload.resize(16000);
                        //     output.write((char *)pkt.payload.data(), 16000);
                        // }

                        // if (pkt.header.apid != 2047)
                        //     logger->info("%d, %d", pkt.header.apid, pkt.payload.size());

                        // if (pkt.header.apid == 18)
                        //{
                        //     pkt.payload.resize(16000);
                        //     output.write((char *)pkt.payload.data(), 16000);
                        // }
                    }
#endif
                }
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
                else if (vcdu.vcid == 4) // Idk VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid4.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (d_satellite == PROBA_V)
                        {
                            // Vegs-1
                            if (pkt.header.apid == 289)
                                vegs_readers[1][0]->work(pkt);
                            else if (pkt.header.apid == 273)
                                vegs_readers[1][1]->work(pkt);
                            else if (pkt.header.apid == 321)
                                vegs_readers[1][2]->work(pkt);
                            else if (pkt.header.apid == 281)
                                vegs_readers[1][3]->work(pkt);
                            else if (pkt.header.apid == 297)
                                vegs_readers[1][4]->work(pkt);
                            else if (pkt.header.apid == 329)
                                vegs_readers[1][5]->work(pkt);
                            // Vegs-2
                            else if (pkt.header.apid == 290)
                                vegs_readers[0][0]->work(pkt);
                            else if (pkt.header.apid == 274)
                                vegs_readers[0][1]->work(pkt);
                            else if (pkt.header.apid == 322)
                                vegs_readers[0][2]->work(pkt);
                            else if (pkt.header.apid == 282)
                                vegs_readers[0][3]->work(pkt);
                            else if (pkt.header.apid == 298)
                                vegs_readers[0][4]->work(pkt);
                            else if (pkt.header.apid == 330)
                                vegs_readers[0][5]->work(pkt);
                            // Vegs-3
                            else if (pkt.header.apid == 292)
                                vegs_readers[2][0]->work(pkt);
                            else if (pkt.header.apid == 276)
                                vegs_readers[2][1]->work(pkt);
                            else if (pkt.header.apid == 324)
                                vegs_readers[2][2]->work(pkt);
                            else if (pkt.header.apid == 284)
                                vegs_readers[2][3]->work(pkt);
                            else if (pkt.header.apid == 300)
                                vegs_readers[2][4]->work(pkt);
                            else if (pkt.header.apid == 332)
                                vegs_readers[2][5]->work(pkt);
                            else
                                logger->critical(pkt.header.apid);
                        }
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

            // CHRIS
            if (d_satellite == PROBA_1)
            {
                chris_status = SAVING;

                logger->info("----------- CHRIS");
                logger->info("Images : %d", chris_reader->cnt());

                chris_reader->save();

                chris_status = DONE;
            }

            // HRC
            if (d_satellite == PROBA_1)
            {
                hrc_status = SAVING;

                logger->info("----------- HRC");
                logger->info("Images : %d", hrc_reader->getCount());

                hrc_reader->save();

                hrc_status = DONE;
            }

            // SWAP
            if (d_satellite == PROBA_2)
            {
                swap_status = SAVING;

                logger->info("----------- SWAP");
                logger->info("Images : %d", swap_reader->count);

                swap_reader->save();

                swap_status = DONE;
            }

            // VegS
            if (d_satellite == PROBA_V)
            {
                for (int i = 0; i < 3; i++)
                {
                    for (int y = 0; y < 6; y++)
                        vegs_status[i][y] = SAVING;

                    logger->info("----------- Vegetation %d", i + 1);
                    logger->info("Lines (1) : %d", vegs_readers[i][0]->lines);
                    logger->info("Lines (2) : %d", vegs_readers[i][1]->lines);
                    logger->info("Lines (3) : %d", vegs_readers[i][2]->lines);
                    logger->info("Lines (4) : %d", vegs_readers[i][3]->lines);
                    logger->info("Lines (5) : %d", vegs_readers[i][4]->lines);
                    logger->info("Lines (6) : %d", vegs_readers[i][5]->lines);

                    std::string vegs_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Vegetation";

                    vegs_readers[i][0]->getImg().save_img(vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_1");
                    vegs_readers[i][1]->getImg().save_img(vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_2");
                    auto img3 = vegs_readers[i][2]->getImg();
                    img3.mirror(true, false);
                    img3.save_img(vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_3");
                    img3.clear();
                    vegs_readers[i][3]->getImg().save_img(vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_4");
                    vegs_readers[i][4]->getImg().save_img(vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_5");
                    vegs_readers[i][5]->getImg().save_img(vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_6");

                    for (int y = 0; y < 6; y++)
                        vegs_status[i][y] = DONE;
                }
            }

            if (d_satellite == PROBA_1)
                dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void PROBAInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Proba Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", chris_reader->cnt());
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(chris_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("HRC");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", hrc_reader->getCount());
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

                if (d_satellite == PROBA_V)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch1", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(ImColor(0, 255, 0), "%d", vegs_readers[i][0]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][0]);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch2", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(ImColor(0, 255, 0), "%d", vegs_readers[i][1]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][1]);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch2", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(ImColor(0, 255, 0), "%d", vegs_readers[i][2]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][2]);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch4", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(ImColor(0, 255, 0), "%d", vegs_readers[i][3]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][3]);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch5", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(ImColor(0, 255, 0), "%d", vegs_readers[i][4]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][4]);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch6", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(ImColor(0, 255, 0), "%d", vegs_readers[i][5]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][5]);
                    }
                }

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

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