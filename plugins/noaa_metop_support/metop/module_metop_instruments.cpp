#include "module_metop_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "metop.h"
#include "common/image/bowtie.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "products/products.h"

namespace metop
{
    namespace instruments
    {
        MetOpInstrumentsDecoderModule::MetOpInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MetOpInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1024];

            // Demuxers
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid3(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid9(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid10(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid12(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid15(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid24(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid34(882, true);

            // Setup Admin Message
            {
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Admin Messages";
                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);
                admin_msg_reader.directory = directory;
            }

            std::vector<uint8_t> metop_scids;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                if (vcdu.spacecraft_id == METOP_A_SCID ||
                    vcdu.spacecraft_id == METOP_B_SCID ||
                    vcdu.spacecraft_id == METOP_C_SCID)
                    metop_scids.push_back(vcdu.spacecraft_id);

                if (vcdu.vcid == 9) // AVHRR/3
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid9.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 103 || pkt.header.apid == 104)
                            avhrr_reader.work(pkt);
                }
                else if (vcdu.vcid == 12) // MHS
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid12.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 34)
                            mhs_reader.work(pkt);
                }
                else if (vcdu.vcid == 15) // ASCAT
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid15.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        ascat_reader.work(pkt);
                }
                else if (vcdu.vcid == 10) // IASI
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid10.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 150)
                            iasi_reader_img.work(pkt);
                        else if (pkt.header.apid == 130 || pkt.header.apid == 135 || pkt.header.apid == 140 || pkt.header.apid == 145)
                            iasi_reader.work(pkt);
                    }
                }
                else if (vcdu.vcid == 3) // AMSU
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid3.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 39)
                            amsu_a1_reader.work(pkt);
                        else if (pkt.header.apid == 40)
                            amsu_a2_reader.work(pkt);
                        else if (pkt.header.apid == 37)
                            sem_reader.work(pkt);
                    }
                }
                else if (vcdu.vcid == 24) // GOME
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid24.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 384)
                            gome_reader.work(pkt);
                }
                else if (vcdu.vcid == 34) // Admin Messages
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid34.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 6)
                            admin_msg_reader.work(pkt);
                }

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            int scid = most_common(metop_scids.begin(), metop_scids.end());
            metop_scids.clear();

            std::string sat_name = "Unknown MetOp";
            if (scid == METOP_A_SCID)
                sat_name = "MetOp-A";
            else if (scid == METOP_B_SCID)
                sat_name = "MetOp-B";
            else if (scid == METOP_C_SCID)
                sat_name = "MetOp-C";

            int norad = 0;
            if (scid == METOP_A_SCID)
                norad = METOP_A_NORAD;
            else if (scid == METOP_B_SCID)
                norad = METOP_B_NORAD;
            else if (scid == METOP_C_SCID)
                norad = METOP_C_NORAD;

            // Satellite ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : " + sat_name);
            }

            // AVHRR
            {
                avhrr_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AVHRR";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- AVHRR/3");
                logger->info("Lines : " + std::to_string(avhrr_reader.lines));

                satdump::ImageProducts avhrr_products;
                avhrr_products.instrument_name = "avhrr_3";
                avhrr_products.has_timestamps = true;
                avhrr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                avhrr_products.set_timestamps(avhrr_reader.timestamps);

                for (int i = 0; i < 5; i++)
                    avhrr_products.images.push_back({"AVHRR-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), avhrr_reader.getChannel(i)});

                avhrr_products.save(directory);

                avhrr_status = DONE;
            }

            // MHS
            {
                mhs_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MHS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MHS");
                logger->info("Lines : " + std::to_string(mhs_reader.lines));

                /*for (int i = 0; i < 5; i++)
                {
                    logger->info("Channel " + std::to_string(i + 1) + "...");
                    WRITE_IMAGE(mhs_reader.getChannel(i), directory + "/MHS-" + std::to_string(i + 1) + ".png");
                }*/

                satdump::ImageProducts mhs_products;
                mhs_products.instrument_name = "mhs";
                mhs_products.has_timestamps = true;
                mhs_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                mhs_products.set_timestamps(mhs_reader.timestamps);

                for (int i = 0; i < 5; i++)
                    mhs_products.images.push_back({"MHS-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), mhs_reader.getChannel(i)});

                mhs_products.save(directory);

                mhs_status = DONE;
            }

            // ASCAT
            {
                ascat_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ASCAT";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- ASCAT");
                for (int i = 0; i < 6; i++)
                    logger->info("Channel " + std::to_string(i + 1) + " lines : " + std::to_string(ascat_reader.lines[i]));

                for (int i = 0; i < 6; i++)
                {
                    logger->info("Channel " + std::to_string(i + 1) + "...");
                    image::Image<uint16_t> img = ascat_reader.getChannel(i);
                    WRITE_IMAGE(img, directory + "/ASCAT-" + std::to_string(i + 1) + ".png");
                }
                ascat_status = DONE;
            }

            // IASI
            {
                iasi_img_status = iasi_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IASI";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- IASI");
                logger->info("Lines : " + std::to_string(iasi_reader.lines));
                logger->info("Lines (Imaging) : " + std::to_string(iasi_reader_img.lines * 64));

                const float alpha = 1.0 / 1.8;
                const float beta = 0.58343;
                const long scanHeight = 64;

                if (iasi_reader.lines > 0)
                {
                    logger->info("Channel IR imaging...");
                    image::Image<uint16_t> iasi_imaging = iasi_reader_img.getIRChannel();
                    iasi_imaging = image::bowtie::correctGenericBowTie(iasi_imaging, 1, scanHeight, alpha, beta); // Bowtie.... As IASI scans per IFOV
                    iasi_imaging.simple_despeckle(10);                                                            // And, it has some dead pixels sometimes so well, we need to remove them I guess?

                    image::Image<uint16_t> iasi_imaging_equ = iasi_imaging;
                    image::Image<uint16_t> iasi_imaging_equ_inv = iasi_imaging;
                    WRITE_IMAGE(iasi_imaging, directory + "/IASI-IMG.png");
                }
                iasi_img_status = DONE;

                // satdump::ImageProducts iasi_products;
                // iasi_products.instrument_name = "iasi";
                //  iasi_products.has_timestamps = true;
                //  iasi_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                //  iasi_products.set_timestamps(iasi_reader.timestamps);

                // for (int i = 0; i < 8461; i++)
                //     iasi_products.images.push_back({"IASI-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), iasi_reader.getChannel(i)});

                // iasi_products.save(directory);

                // Output a few nice composites as well
                logger->info("Global Composite...");
                image::Image<uint16_t> imageAll = image::make_manyimg_composite<uint16_t>(150, 56, 8461, [this](int c)
                                                                                          { return iasi_reader.getChannel(c); });
                WRITE_IMAGE(imageAll, directory + "/IASI-ALL.png");
                iasi_status = DONE;
            }

            // AMSU
            {
                amsu_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AMSU";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- AMSU");
                logger->info("Lines (AMSU A1) : " + std::to_string(amsu_a1_reader.lines));
                logger->info("Lines (AMSU A2) : " + std::to_string(amsu_a2_reader.lines));

                for (int i = 0; i < 2; i++)
                {
                    logger->info("Channel " + std::to_string(i + 1) + "...");
                    WRITE_IMAGE(amsu_a2_reader.getChannel(i), directory + "/AMSU-A2-" + std::to_string(i + 1) + ".png");
                }

                for (int i = 0; i < 13; i++)
                {
                    logger->info("Channel " + std::to_string(i + 3) + "...");
                    WRITE_IMAGE(amsu_a1_reader.getChannel(i), directory + "/AMSU-A1-" + std::to_string(i + 3) + ".png");
                }
                amsu_status = DONE;
            }

            // GOME
            {
                gome_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/GOME";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- GOME");
                logger->info("Lines : " + std::to_string(gome_reader.lines));

                // Output a few nice composites as well
                logger->info("Global Composite...");
                image::Image<uint16_t> imageAll = image::make_manyimg_composite<uint16_t>(130, 48, 6144, [this](int c)
                                                                                          { return gome_reader.getChannel(c); });
                WRITE_IMAGE(imageAll, directory + "/GOME-ALL.png");
                imageAll.clear();
                gome_status = DONE;
            }

            // Admin Messages
            {
                admin_msg_status = DONE;
                logger->info("----------- Admin Message");
                logger->info("Count : " + std::to_string(admin_msg_reader.count));
            }

            // TODO SEM
        }

        void MetOpInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp Instruments Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##metopinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
                ImGui::Text("AVHRR");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", avhrr_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(avhrr_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("IASI");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", iasi_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(iasi_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("IASI Imaging");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", iasi_reader_img.lines * 64);
                ImGui::TableSetColumnIndex(2);
                drawStatus(iasi_img_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MHS");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", mhs_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(mhs_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AMSU A1");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", amsu_a1_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(amsu_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AMSU A2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", amsu_a2_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(amsu_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("GOME");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", gome_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(gome_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("ASCAT");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", ascat_reader.lines[0]);
                ImGui::TableSetColumnIndex(2);
                drawStatus(gome_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("SEM");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", sem_reader.samples);
                ImGui::TableSetColumnIndex(2);
                drawStatus(sem_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Admin Messages");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", admin_msg_reader.count);
                ImGui::TableSetColumnIndex(2);
                drawStatus(admin_msg_status);

                ImGui::EndTable();
            }

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpInstrumentsDecoderModule::getID()
        {
            return "metop_instruments";
        }

        std::vector<std::string> MetOpInstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MetOpInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MetOpInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop