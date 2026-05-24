#include "module_proba_instruments.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "core/exception.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "instruments/vegetation/vegx_reader.h"
#include "logger.h"
#include "products/dataset.h"
#include "products/image/channel_transform.h"
#include "products/image_product.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>

namespace proba
{
    namespace instruments
    {
        PROBAInstrumentsDecoderModule::PROBAInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            if (parameters["satellite"] == "proba1")
                d_satellite = PROBA_1;
            else if (parameters["satellite"] == "proba2")
                d_satellite = PROBA_2;
            else if (parameters["satellite"] == "probaV")
                d_satellite = PROBA_V;
            else if (parameters["satellite"] == "proba3")
                d_satellite = PROBA_3;
            else
                throw satdump_exception("Proba Instruments Decoder : Proba satellite \"" + parameters["satellite"].get<std::string>() + "\" is not valid!");
            fsfsm_enable_output = false;
        }

        void PROBAInstrumentsDecoderModule::process()
        {
            uint8_t cadu[1279];

            // Demuxers
            ccsds::ccsds_tm::Demuxer demuxer_vcid1(1103, false);
            ccsds::ccsds_tm::Demuxer demuxer_vcid2(1103, false);
            ccsds::ccsds_tm::Demuxer demuxer_vcid3(1103, false);
            ccsds::ccsds_tm::Demuxer demuxer_vcid4(1103, false);

            // std::ofstream output("file.ccsds");

            // Products dataset
            satdump::products::DataSet dataset;
            if (d_satellite == PROBA_1)
                dataset.satellite_name = "PROBA-1";
            else if (d_satellite == PROBA_2)
                dataset.satellite_name = "PROBA-2";
            else if (d_satellite == PROBA_V)
                dataset.satellite_name = "PROBA-V";
            else if (d_satellite == PROBA_3)
                dataset.satellite_name = "PROBA-3";
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
                std::string vegs_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Vegetation-S";

                if (!std::filesystem::exists(vegs_directory))
                    std::filesystem::create_directory(vegs_directory);

                std::string vegx_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Vegetation-X";

                if (!std::filesystem::exists(vegx_directory))
                    std::filesystem::create_directory(vegx_directory);

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

            int i = 0;

            // bool is_pv_x = true;

            vegetation::VegetationX vegx_reader1(352, 128, 15);
            vegetation::VegetationX vegx_reader2(352, 128, 3);
            vegetation::VegetationX vegx_reader3(352, 128, 3);
            vegetation::VegetationX vegx_reader4(352, 128, 3);
            vegetation::VegetationX vegx_reader5(352, 128, 15);
            vegetation::VegetationX vegx_reader6(352, 128, 15);
            vegetation::VegetationX vegx_reader7(352, 128, 15);
            vegetation::VegetationX vegx_reader8(352, 128, 15);
            vegetation::VegetationX vegx_reader9(352, 128, 3);
            vegetation::VegetationX vegx_reader10(352, 128, 3);
            vegetation::VegetationX vegx_reader11(352, 128, 3);
            vegetation::VegetationX vegx_reader12(352, 128, 15);
            vegetation::VegetationX vegx_reader13(352, 128, 15);
            vegetation::VegetationX vegx_reader14(352, 128, 15);
            vegetation::VegetationX vegx_reader15(352, 128, 15);
            vegetation::VegetationX vegx_reader16(352, 128, 3);
            vegetation::VegetationX vegx_reader17(352, 128, 3);
            vegetation::VegetationX vegx_reader18(352, 128, 3);

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1279);

                // Parse this transport frame
                ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

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
                            // if (!is_pv_x)
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
                            //  else
                            {
                                if (pkt.header.apid == 324)
                                    vegx_reader1.work(pkt);
                                else if (pkt.header.apid == 330)
                                    vegx_reader2.work(pkt);
                                else if (pkt.header.apid == 298)
                                    vegx_reader3.work(pkt);
                                else if (pkt.header.apid == 329)
                                    vegx_reader4.work(pkt);
                                else if (pkt.header.apid == 289)
                                    vegx_reader5.work(pkt);
                                else if (pkt.header.apid == 273)
                                    vegx_reader6.work(pkt);
                                else if (pkt.header.apid == 321)
                                    vegx_reader7.work(pkt);
                                else if (pkt.header.apid == 290)
                                    vegx_reader8.work(pkt);
                                else if (pkt.header.apid == 282)
                                    vegx_reader9.work(pkt);
                                else if (pkt.header.apid == 297)
                                    vegx_reader10.work(pkt);
                                else if (pkt.header.apid == 281)
                                    vegx_reader11.work(pkt);
                                else if (pkt.header.apid == 274)
                                    vegx_reader12.work(pkt);
                                else if (pkt.header.apid == 322)
                                    vegx_reader13.work(pkt);
                                else if (pkt.header.apid == 292)
                                    vegx_reader14.work(pkt);
                                else if (pkt.header.apid == 276)
                                    vegx_reader15.work(pkt);
                                else if (pkt.header.apid == 332)
                                    vegx_reader16.work(pkt);
                                else if (pkt.header.apid == 300)
                                    vegx_reader17.work(pkt);
                                else if (pkt.header.apid == 284)
                                    vegx_reader18.work(pkt);
                                else
                                    logger->info("%d, %d", pkt.header.apid, pkt.payload.size());
                            }
                        }
                        else if (d_satellite == PROBA_3) {}
                    }
                }
            }

            cleanup();

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

                    // if (!is_pv_x)
                    {
                        std::string vegs_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Vegetation-S";

                        logger->info("----------- Vegetation %d", i + 1);
                        logger->info("Lines (1) : %d", vegs_readers[i][0]->lines);
                        logger->info("Lines (2) : %d", vegs_readers[i][1]->lines);
                        logger->info("Lines (3) : %d", vegs_readers[i][2]->lines);
                        logger->info("Lines (4) : %d", vegs_readers[i][3]->lines);
                        logger->info("Lines (5) : %d", vegs_readers[i][4]->lines);
                        logger->info("Lines (6) : %d", vegs_readers[i][5]->lines);

                        auto img = vegs_readers[i][0]->getImg();
                        image::save_img(img, vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_1");
                        img = vegs_readers[i][1]->getImg();
                        image::save_img(img, vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_2");
                        auto img3 = vegs_readers[i][2]->getImg();
                        img3.mirror(true, false);
                        image::save_img(img3, vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_3");
                        img3.clear();
                        img = vegs_readers[i][3]->getImg();
                        image::save_img(img, vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_4");
                        img = vegs_readers[i][4]->getImg();
                        image::save_img(img, vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_5");
                        img = vegs_readers[i][5]->getImg();
                        image::save_img(img, vegs_directory + "/Vegetation" + std::to_string(i + 1) + "_6");
                    }
                    //  else
                    {
                        std::string vegs_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Vegetation-X";

                        for (auto &tocorr_inst : std::vector<std::vector<vegetation::VegetationX *>>{
                                 {&vegx_reader1, &vegx_reader14, &vegx_reader15}, //
                                 {&vegx_reader8, &vegx_reader12, &vegx_reader13}, //
                                 {&vegx_reader5, &vegx_reader6, &vegx_reader7},   //
                                 //
                                 {&vegx_reader16, &vegx_reader17, &vegx_reader18}, //
                                 {&vegx_reader2, &vegx_reader3, &vegx_reader9},    //
                                 {&vegx_reader4, &vegx_reader10, &vegx_reader11},  //
                             })
                        {
                            std::vector<int> to_corr;

                            for (auto &veg : tocorr_inst)
                                for (auto &seg : veg->img_segments)
                                    if (std::find(to_corr.begin(), to_corr.end(), seg.first) == to_corr.end())
                                        to_corr.push_back(seg.first);

                            std::sort(to_corr.begin(), to_corr.end());

                            for (auto &veg : tocorr_inst)
                            {
                                auto vegcpy = veg->img_segments;
                                veg->img_segments.erase(veg->img_segments.begin(), veg->img_segments.end());

                                for (auto &line : to_corr)
                                {
                                    if (vegcpy.count(line))
                                        veg->img_segments.insert_or_assign(line, vegcpy.find(line)->second);
                                    else
                                        veg->img_segments.insert_or_assign(line, std::array<satdump::image::Image, 15>());
                                }
                            }
                        }

                        {
                            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Vegetation-X/Vegetation_1";

                            if (!std::filesystem::exists(directory))
                                std::filesystem::create_directories(directory);

                            satdump::products::ImageProduct ocm_products;
                            ocm_products.instrument_name = "probav_vegetation_hd";

                            ocm_products.images.push_back({0, "Vegetation-1", "1", vegx_reader1.getImg(), 12});
                            auto img = vegx_reader14.getImg();
                            img.mirror(1, 0);
                            ocm_products.images.push_back({1, "Vegetation-2", "2", img, 12, satdump::ChannelTransform().init_affine_slantx(0.996000, 1, -70.000000, -160.000000, 0, 0.022000)});
                            img = vegx_reader15.getImg();
                            img.mirror(1, 0);
                            ocm_products.images.push_back({2, "Vegetation-3", "3", img, 12, satdump::ChannelTransform().init_affine(1, 1, -80.098282, -66.785806)});

                            ocm_products.save(directory);
                        }

                        {
                            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Vegetation-X/Vegetation_2";

                            if (!std::filesystem::exists(directory))
                                std::filesystem::create_directories(directory);

                            satdump::products::ImageProduct ocm_products;
                            ocm_products.instrument_name = "probav_vegetation_hd";

                            ocm_products.images.push_back({0, "Vegetation-1", "1", vegx_reader13.getImg(), 12});
                            auto img = vegx_reader12.getImg();
                            img.mirror(1, 0);
                            ocm_products.images.push_back({1, "Vegetation-2", "2", img, 12, satdump::ChannelTransform().init_affine(1, 1, -80.021092, -53.112902)});
                            img = vegx_reader8.getImg();
                            img.mirror(1, 0);
                            ocm_products.images.push_back({2, "Vegetation-3", "3", img, 12, satdump::ChannelTransform().init_affine(1, 1, -81.243169, -159.856325)});

                            ocm_products.save(directory);
                        }

                        {
                            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Vegetation-X/Vegetation_3";

                            if (!std::filesystem::exists(directory))
                                std::filesystem::create_directories(directory);

                            satdump::products::ImageProduct ocm_products;
                            ocm_products.instrument_name = "probav_vegetation_hd";

                            ocm_products.images.push_back({0, "Vegetation-1", "1", vegx_reader7.getImg(), 12});
                            auto img = vegx_reader6.getImg();
                            img.mirror(1, 0);
                            ocm_products.images.push_back({1, "Vegetation-2", "2", img, 12, satdump::ChannelTransform().init_affine_slantx(0.999000, 1, -76.000000, -93.000000, 0, -0.007500)});
                            img = vegx_reader5.getImg();
                            img.mirror(1, 0);
                            ocm_products.images.push_back({2, "Vegetation-3", "3", img, 12, satdump::ChannelTransform().init_affine_slantx(0.996500, 1, -69.000000, -276.000000, 0, -0.023500)});

                            ocm_products.save(directory);
                        }

                        logger->info("----------- Vegetation-X");
                        auto img = vegx_reader1.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_1.png");
                        img = vegx_reader2.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_2.png");
                        img = vegx_reader3.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_3.png");
                        img = vegx_reader4.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_4.png");
                        img = vegx_reader5.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_5.png");
                        img = vegx_reader6.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_6.png");
                        img = vegx_reader7.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_7.png");
                        img = vegx_reader8.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_8.png");
                        img = vegx_reader9.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_9.png");
                        img = vegx_reader10.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_10.png");
                        img = vegx_reader11.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_11.png");
                        img = vegx_reader12.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_12.png");
                        img = vegx_reader13.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_13.png");
                        img = vegx_reader14.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_14.png");
                        img = vegx_reader15.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_15.png");
                        img = vegx_reader16.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_16.png");
                        img = vegx_reader17.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_17.png");
                        img = vegx_reader18.getImg();
                        image::save_img(img, vegs_directory + "/Vegetation_18.png");
                    }

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
                    ImGui::TextColored(style::theme.green, "%d", chris_reader->cnt());
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(chris_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("HRC");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", hrc_reader->getCount());
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(hrc_status);
                }

                if (d_satellite == PROBA_2)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("SWAP");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", swap_reader->count);
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
                        ImGui::TextColored(style::theme.green, "%d", vegs_readers[i][0]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][0]);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch2", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(style::theme.green, "%d", vegs_readers[i][1]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][1]);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch2", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(style::theme.green, "%d", vegs_readers[i][2]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][2]);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch4", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(style::theme.green, "%d", vegs_readers[i][3]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][3]);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch5", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(style::theme.green, "%d", vegs_readers[i][4]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][4]);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Vegetation %d Ch6", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(style::theme.green, "%d", vegs_readers[i][5]->lines);
                        ImGui::TableSetColumnIndex(2);
                        drawStatus(vegs_status[i][5]);
                    }
                }

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string PROBAInstrumentsDecoderModule::getID() { return "proba_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> PROBAInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        { return std::make_shared<PROBAInstrumentsDecoderModule>(input_file, output_file_hint, parameters); }
    } // namespace instruments
} // namespace proba