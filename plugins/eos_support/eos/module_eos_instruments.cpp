#include "module_eos_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_weather/demuxer.h"
#include "common/ccsds/ccsds_weather/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/bowtie.h"
#include "common/utils.h"
#include "products/image_products.h"
#include "products/dataset.h"
#include "resources.h"
#include "instruments/modis/modis_histmatch.h"

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
            ccsds::ccsds_weather::Demuxer demuxer_vcid3;
            ccsds::ccsds_weather::Demuxer demuxer_vcid10;
            ccsds::ccsds_weather::Demuxer demuxer_vcid15;
            ccsds::ccsds_weather::Demuxer demuxer_vcid20;
            ccsds::ccsds_weather::Demuxer demuxer_vcid25;
            ccsds::ccsds_weather::Demuxer demuxer_vcid26;
            ccsds::ccsds_weather::Demuxer demuxer_vcid30;
            ccsds::ccsds_weather::Demuxer demuxer_vcid35;
            ccsds::ccsds_weather::Demuxer demuxer_vcid42;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

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
                    if (vcdu.vcid == 3) // GBAD
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid3.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 957)
                                gbad_reader.work(pkt);
                    }
                    else if (vcdu.vcid == 30) // MODIS
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
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();

            // Products dataset
            satdump::ProductDataSet dataset;
            if (d_satellite == AQUA)
                dataset.satellite_name = "Aqua";
            else if (d_satellite == TERRA)
                dataset.satellite_name = "Terra";
            else if (d_satellite == AURA)
                dataset.satellite_name = "Aura";

            std::optional<satdump::TLE> satellite_tle;
            if (d_satellite == AQUA)
                satellite_tle = satdump::general_tle_registry.get_from_norad(27424);
            else if (d_satellite == TERRA)
                satellite_tle = satdump::general_tle_registry.get_from_norad(25994);
            else if (d_satellite == AURA)
                satellite_tle = satdump::general_tle_registry.get_from_norad(28376);

            if (d_satellite == AQUA || d_satellite == TERRA) // MODIS
            {
                dataset.timestamp = get_median(modis_reader.timestamps_1000);

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

                satdump::ImageProducts modis_products;
                modis_products.instrument_name = "modis";
                modis_products.has_timestamps = true;
                modis_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_IFOV;
                modis_products.set_tle(satellite_tle);
                modis_products.set_timestamps(modis_reader.timestamps_250);
                nlohmann::json proj_cfg;
                if (d_satellite == AQUA)
                    proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/aqua_modis.json"));
                else if (d_satellite == TERRA)
                    proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/terra_modis.json"));
                // if (d_satellite == AQUA)
                //     proj_cfg["ephemeris"] = gbad_reader.getEphem();
                modis_products.set_proj_cfg(proj_cfg);

                for (int i = 0; i < 2; i++)
                {
                    image::Image<uint16_t> image = modis_reader.getImage250m(i);
                    // modis::modis_match_detector_histograms(image, 1 /*4*/, 40 * 2);
                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_250, alpha, beta);
                    modis_products.images.push_back({"MODIS-" + std::to_string(i + 1), std::to_string(i + 1), image});
                }

                for (int i = 0; i < 5; i++)
                {
                    image::Image<uint16_t> image = modis_reader.getImage500m(i);
                    // modis::modis_match_detector_histograms(image, 1 /*2*/, 20 * 2);
                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_500, alpha, beta);
                    modis_products.images.push_back({"MODIS-" + std::to_string(i + 3), std::to_string(i + 3), image});
                }

                for (int i = 0; i < 31; i++)
                {
                    image::Image<uint16_t> image = modis_reader.getImage1000m(i);

                    // modis::modis_match_detector_histograms(image, 1, 10 * 2);

                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_1000, alpha, beta);

                    if (i < 5)
                        modis_products.images.push_back({"MODIS-" + std::to_string(i + 8), std::to_string(i + 8), image});
                    else if (i == 5)
                        modis_products.images.push_back({"MODIS-13L", "13L", image});
                    else if (i == 6)
                        modis_products.images.push_back({"MODIS-13H", "13H", image});
                    else if (i == 7)
                        modis_products.images.push_back({"MODIS-14L", "13L", image});
                    else if (i == 8)
                        modis_products.images.push_back({"MODIS-14H", "14H", image});
                    else
                        modis_products.images.push_back({"MODIS-" + std::to_string(i + 6), std::to_string(i + 6), image});
                }

                modis_products.save(directory);
                dataset.products_list.push_back("MODIS");

                modis_status = DONE;
            }

            if (d_satellite == AQUA) // AIRS
            {
                airs_status = SAVING;
                std::string directory_hd = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AIRS/HD";
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AIRS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);
                if (!std::filesystem::exists(directory_hd))
                    std::filesystem::create_directory(directory_hd);

                logger->info("----------- AIRS");
                logger->info("Lines : " + std::to_string(airs_reader.lines));

                satdump::ImageProducts airs_hd_products;
                airs_hd_products.instrument_name = "airs_hd";
                airs_hd_products.has_timestamps = true;
                airs_hd_products.bit_depth = 16;
                airs_hd_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_IFOV;
                airs_hd_products.set_tle(satellite_tle);
                // airs_hd_products.set_timestamps(airs_reader.timestamps_ifov);

                for (int i = 0; i < 4; i++)
                    airs_hd_products.images.push_back({"AIRS-HD-" + std::to_string(i + 1), std::to_string(i + 1), airs_reader.getHDChannel(i)});

                airs_hd_products.save(directory_hd);
                dataset.products_list.push_back("AIRS/HD");

                satdump::ImageProducts airs_products;
                airs_products.instrument_name = "airs";
                airs_products.has_timestamps = true;
                airs_products.bit_depth = 16;
                airs_products.save_as_matrix = true;
                airs_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                airs_products.set_tle(satellite_tle);
                // airs_products.set_timestamps(airs_reader.timestamps_ifov);

                for (int i = 0; i < 2666; i++)
                    airs_products.images.push_back({"AIRS-" + std::to_string(i + 1), std::to_string(i + 1), airs_reader.getChannel(i)});

                airs_products.save(directory);
                dataset.products_list.push_back("AIRS");

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

                satdump::ImageProducts amsu_products;
                amsu_products.instrument_name = "amsu_a";
                amsu_products.has_timestamps = true;
                amsu_products.set_tle(satellite_tle);
                amsu_products.bit_depth = 16;
                amsu_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                // amsu_products.set_timestamps(amsu_reader.timestamps);
                amsu_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/aqua_amsu.json")));

                for (int i = 0; i < 2; i++)
                    amsu_products.images.push_back({"AMSU-A2-" + std::to_string(i + 1), std::to_string(i + 1), amsu_a2_reader.getChannel(i), amsu_a2_reader.timestamps});

                for (int i = 0; i < 13; i++)
                    amsu_products.images.push_back({"AMSU-A1-" + std::to_string(i + 1), std::to_string(i + 3), amsu_a1_reader.getChannel(i), amsu_a1_reader.timestamps});

                amsu_products.save(directory);
                dataset.products_list.push_back("AMSU");

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

                WRITE_IMAGE(ceres_fm3_reader.getImage(0), directory + "/CERES1-SHORTWAVE");
                WRITE_IMAGE(ceres_fm3_reader.getImage(1), directory + "/CERES1-LONGWAVE");
                WRITE_IMAGE(ceres_fm3_reader.getImage(2), directory + "/CERES1-TOTAL");
                ceres_fm3_status = DONE;

                WRITE_IMAGE(ceres_fm4_reader.getImage(0), directory + "/CERES2-SHORTWAVE");
                WRITE_IMAGE(ceres_fm4_reader.getImage(1), directory + "/CERES2-LONGWAVE");
                WRITE_IMAGE(ceres_fm4_reader.getImage(2), directory + "/CERES2-TOTAL");
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

                WRITE_IMAGE(omi_1_reader.getImageRaw(), directory + "/OMI-1");
                WRITE_IMAGE(omi_2_reader.getImageRaw(), directory + "/OMI-2");

                WRITE_IMAGE(omi_1_reader.getImageVisible(), directory + "/OMI-VIS-1");
                WRITE_IMAGE(omi_2_reader.getImageVisible(), directory + "/OMI-VIS-2");

                image::Image<uint16_t> imageAll1 = image::make_manyimg_composite<uint16_t>(33, 24, 792, [this](int c)
                                                                                           { return omi_1_reader.getChannel(c); });
                image::Image<uint16_t> imageAll2 = image::make_manyimg_composite<uint16_t>(33, 24, 792, [this](int c)
                                                                                           { return omi_2_reader.getChannel(c); });
                WRITE_IMAGE(imageAll1, directory + "/OMI-ALL-1");
                WRITE_IMAGE(imageAll2, directory + "/OMI-ALL-2");
                omi_status = DONE;
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void EOSInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("EOS Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

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