#include "module_eos_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/bowtie.h"
#include "common/utils.h"

namespace eos
{
    namespace instruments
    {
        EOSInstrumentsDecoderModule::EOSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters),
              d_modis_bowtie(d_parameters["modis_bowtie"].get<bool>())
        {
            if (parameters["satellite"] == "terra")
                d_satellite = TERRA;
            else if (parameters["satellite"] == "aqua")
                d_satellite = AQUA;
            else if (parameters["satellite"] == "aura")
                d_satellite = AURA;
            else
                throw std::runtime_error("EOS Instruments Decoder : EOS satellite \"" + parameters["satellite"].get<std::string>() + "\" is not valid!");
        }

        void EOSInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MODIS";

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1024];

            // Demuxers
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid10;
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid15;
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid20;
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid25;
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid26;
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid30;
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid35;
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid42;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                if (d_satellite == TERRA)
                {
                    if (vcdu.vcid == 42) // MODIS
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid42.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 64)
                                modis_reader.work(pkt);
                    }
                }
                else if (d_satellite == AQUA)
                {
                    if (vcdu.vcid == 30) // MODIS
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid30.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 64)
                                modis_reader.work(pkt);
                    }
                    else if (vcdu.vcid == 35) // AIRS
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid35.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 404)
                                airs_reader.work(pkt);
                    }
                    else if (vcdu.vcid == 20) // AMSU-A1
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid20.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 261 || pkt.header.apid == 262)
                                amsu_a1_reader.work(pkt);
                    }
                    else if (vcdu.vcid == 25) // AMSU-A2
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid25.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 290)
                                amsu_a2_reader.work(pkt);
                    }

                    else if (vcdu.vcid == 10) // CERES FM-3
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid10.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 141)
                                ceres_fm3_reader.work(pkt);
                    }
                    else if (vcdu.vcid == 15) // CERES FM-4
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid15.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 157)
                                ceres_fm4_reader.work(pkt);
                    }
                }
                else if (d_satellite == AURA)
                {
                    if (vcdu.vcid == 26) // OMI
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid26.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        {
                            if (pkt.header.apid == 1838)
                                omi_1_reader.work(pkt);
                            else if (pkt.header.apid == 1840)
                                omi_2_reader.work(pkt);
                        }
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

            if (d_satellite == AQUA || d_satellite == TERRA) // MODIS
            {
                modis_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MODIS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MODIS");
                logger->info("Lines (1km) : " + std::to_string(modis_reader.lines));

                // BowTie values
                const float alpha = 1.0 / 1.8;
                const float beta = 0.58333;
                const long scanHeight_250 = 40;
                const long scanHeight_500 = 20;
                const long scanHeight_1000 = 10;

                for (int i = 0; i < 2; i++)
                {
                    image::Image<uint16_t> image = modis_reader.getImage250m(i);

                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_250, alpha, beta);

                    logger->info("Channel " + std::to_string(i + 1) + "...");
                    WRITE_IMAGE(image, directory + "/MODIS-" + std::to_string(i + 1) + ".png");
                }

                for (int i = 0; i < 5; i++)
                {
                    image::Image<uint16_t> image = modis_reader.getImage500m(i);

                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_500, alpha, beta);

                    logger->info("Channel " + std::to_string(i + 3) + "...");
                    WRITE_IMAGE(image, directory + "/MODIS-" + std::to_string(i + 3) + ".png");
                }

                for (int i = 0; i < 31; i++)
                {
                    image::Image<uint16_t> image = modis_reader.getImage1000m(i);

                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_1000, alpha, beta);

                    if (i < 5)
                    {
                        logger->info("Channel " + std::to_string(i + 8) + "...");
                        WRITE_IMAGE(image, directory + "/MODIS-" + std::to_string(i + 8) + ".png");
                    }
                    else if (i == 5)
                    {
                        logger->info("Channel 13L...");
                        WRITE_IMAGE(image, directory + "/MODIS-13L.png");
                    }
                    else if (i == 6)
                    {
                        logger->info("Channel 13H...");
                        WRITE_IMAGE(image, directory + "/MODIS-13H.png");
                    }
                    else if (i == 7)
                    {
                        logger->info("Channel 14L...");
                        WRITE_IMAGE(image, directory + "/MODIS-14L.png");
                    }
                    else if (i == 8)
                    {
                        logger->info("Channel 14H...");
                        WRITE_IMAGE(image, directory + "/MODIS-14H.png");
                    }
                    else
                    {
                        logger->info("Channel " + std::to_string(i + 6) + "...");
                        WRITE_IMAGE(image, directory + "/MODIS-" + std::to_string(i + 6) + ".png");
                    }
                }
                modis_status = DONE;
            }

            if (d_satellite == AQUA) // AIRS
            {
                airs_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AIRS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- AIRS");
                logger->info("Lines : " + std::to_string(airs_reader.lines));

                for (int i = 0; i < 4; i++)
                {
                    logger->info("HD Channel " + std::to_string(i + 1) + "...");
                    WRITE_IMAGE(airs_reader.getHDChannel(i), directory + "/AIRS-HD-" + std::to_string(i + 1) + ".png");
                }

                // There nearly 3000 channels... So we write that in a specific folder not to fill up the main one
                // if (!std::filesystem::exists(directory + "/Channels"))
                //    std::filesystem::create_directory(directory + "/Channels");

                // for (int i = 0; i < 2666; i++)
                //{
                //     logger->info("Channel " + std::to_string(i + 1) + "...");
                //     WRITE_IMAGE(airs_reader.getChannel(i), directory + "/Channels/AIRS-" + std::to_string(i + 1) + ".png");
                // }

                image::Image<uint16_t> imageAll = image::make_manyimg_composite<uint16_t>(100, 27, 2666, [this](int c)
                                                                                          { return airs_reader.getChannel(c); });
                WRITE_IMAGE(imageAll, directory + "/AIRS-ALL.png");
                airs_status = DONE;
            }

            if (d_satellite == AQUA) // AMSU
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

            if (d_satellite == AQUA) // CERES FM-3 and FM-4
            {
                ceres_fm3_status = ceres_fm4_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CERES";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- CERES");
                logger->info("Lines (FM3) : " + std::to_string(ceres_fm3_reader.lines));
                logger->info("Lines (FM4) : " + std::to_string(ceres_fm4_reader.lines));

                logger->info("Shortwave Channel 1...");
                WRITE_IMAGE(ceres_fm3_reader.getImage(0), directory + "/CERES1-SHORTWAVE.png");
                logger->info("Longwave Channel 1...");
                WRITE_IMAGE(ceres_fm3_reader.getImage(1), directory + "/CERES1-LONGWAVE.png");
                logger->info("Total Channel 1...");
                WRITE_IMAGE(ceres_fm3_reader.getImage(2), directory + "/CERES1-TOTAL.png");
                ceres_fm3_status = DONE;

                logger->info("Shortwave Channel 2...");
                WRITE_IMAGE(ceres_fm4_reader.getImage(0), directory + "/CERES2-SHORTWAVE.png");
                logger->info("Longwave Channel 2...");
                WRITE_IMAGE(ceres_fm4_reader.getImage(1), directory + "/CERES2-LONGWAVE.png");
                logger->info("Total Channel 2...");
                WRITE_IMAGE(ceres_fm4_reader.getImage(2), directory + "/CERES2-TOTAL.png");
                ceres_fm4_status = DONE;
            }

            if (d_satellite == AURA) // OMI
            {
                omi_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/OMI";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- OMI");
                logger->info("Lines (UV) : " + std::to_string(omi_1_reader.lines));
                logger->info("Lines (VIS) : " + std::to_string(omi_2_reader.lines));

                WRITE_IMAGE(omi_1_reader.getImageRaw(), directory + "/OMI-1.png");
                WRITE_IMAGE(omi_2_reader.getImageRaw(), directory + "/OMI-2.png");

                WRITE_IMAGE(omi_1_reader.getImageVisible(), directory + "/OMI-VIS-1.png");
                WRITE_IMAGE(omi_2_reader.getImageVisible(), directory + "/OMI-VIS-2.png");

                image::Image<uint16_t> imageAll1 = image::make_manyimg_composite<uint16_t>(33, 24, 792, [this](int c)
                                                                                           { return omi_1_reader.getChannel(c); });
                image::Image<uint16_t> imageAll2 = image::make_manyimg_composite<uint16_t>(33, 24, 792, [this](int c)
                                                                                           { return omi_2_reader.getChannel(c); });
                WRITE_IMAGE(imageAll1, directory + "/OMI-ALL-1.png");
                WRITE_IMAGE(imageAll2, directory + "/OMI-ALL-2.png");
                omi_status = DONE;
            }
        }

        void EOSInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("EOS Instruments Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##eosinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Lines / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                if (d_satellite == TERRA || d_satellite == AQUA)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MODIS");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", modis_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(modis_status);
                }

                if (d_satellite == AQUA)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("AIRS");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", airs_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(airs_status);

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
                    ImGui::Text("CERES FM-3");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", ceres_fm3_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(ceres_fm3_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("CERES FM-4");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", ceres_fm4_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(ceres_fm4_status);
                }

                if (d_satellite == AURA)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("OMI 1");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", omi_1_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(omi_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("OMI 2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", omi_2_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(omi_status);
                }

                ImGui::EndTable();
            }

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string EOSInstrumentsDecoderModule::getID()
        {
            return "eos_instruments";
        }

        std::vector<std::string> EOSInstrumentsDecoderModule::getParameters()
        {
            return {"satellite", "modis_bowtie"};
        }

        std::shared_ptr<ProcessingModule> EOSInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<EOSInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace modis
} // namespace eos