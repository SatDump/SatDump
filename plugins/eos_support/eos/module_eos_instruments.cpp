#include "module_eos_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/bowtie.h"
#include "common/utils.h"
#include "products/image_products.h"
#include "products/dataset.h"
#include "resources.h"
#include "instruments/modis/modis_histmatch.h"
#include "nlohmann/json_utils.h"

#include "common/calibration.h"
#include "instruments/modis/calibrator/modis_calibrator.h"
#include "core/exception.h"

#include "common/image/io.h"
#include "common/image/image_utils.h"

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
                throw satdump_exception("EOS Instruments Decoder : EOS satellite \"" + parameters["satellite"].get<std::string>() + "\" is not valid!");
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
            ccsds::ccsds_aos::Demuxer demuxer_vcid3;
            ccsds::ccsds_aos::Demuxer demuxer_vcid10;
            ccsds::ccsds_aos::Demuxer demuxer_vcid15;
            ccsds::ccsds_aos::Demuxer demuxer_vcid20;
            ccsds::ccsds_aos::Demuxer demuxer_vcid25;
            ccsds::ccsds_aos::Demuxer demuxer_vcid26;
            ccsds::ccsds_aos::Demuxer demuxer_vcid30;
            ccsds::ccsds_aos::Demuxer demuxer_vcid35;
            ccsds::ccsds_aos::Demuxer demuxer_vcid42;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

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
                       // std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid15.work(cadu);
                       // for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                           // if (pkt.header.apid == 157)
                            //    ceres_fm4_reader.work(pkt);
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

            if (d_satellite == AQUA || d_satellite == TERRA) // MODIS
                dataset.timestamp = get_median(modis_reader.timestamps_1000);
            else
                dataset.timestamp = time(0);

            std::optional<satdump::TLE> satellite_tle;
            if (d_satellite == AQUA)
                satellite_tle = satdump::general_tle_registry.get_from_norad_time(27424, dataset.timestamp);
            else if (d_satellite == TERRA)
                satellite_tle = satdump::general_tle_registry.get_from_norad_time(25994, dataset.timestamp);
            else if (d_satellite == AURA)
                satellite_tle = satdump::general_tle_registry.get_from_norad_time(28376, dataset.timestamp);

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

                satdump::ImageProducts modis_products;
                modis_products.instrument_name = "modis";
                modis_products.has_timestamps = true;
                modis_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_IFOV;
                modis_products.bit_depth = 12;
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
                    image::Image image = modis_reader.getImage250m(i);
                    // modis::modis_match_detector_histograms(image, 1 /*4*/, 40 * 2);
                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_250, alpha, beta);
                    modis_products.images.push_back({"MODIS-" + std::to_string(i + 1), std::to_string(i + 1), image});
                }

                for (int i = 0; i < 5; i++)
                {
                    image::Image image = modis_reader.getImage500m(i);
                    // modis::modis_match_detector_histograms(image, 1 /*2*/, 20 * 2);
                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_500, alpha, beta);
                    modis_products.images.push_back({"MODIS-" + std::to_string(i + 3), std::to_string(i + 3), image});
                }

                std::vector<std::vector<int>> bowtie_lut_1km;
                for (int i = 0; i < 31; i++)
                {
                    image::Image image = modis_reader.getImage1000m(i);

                    // modis::modis_match_detector_histograms(image, 1, 10 * 2);

                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_1000, alpha, beta, &bowtie_lut_1km);

                    if (i < 5)
                        modis_products.images.push_back({"MODIS-" + std::to_string(i + 8), std::to_string(i + 8), image});
                    else if (i == 5)
                        modis_products.images.push_back({"MODIS-13L", "13L", image});
                    else if (i == 6)
                        modis_products.images.push_back({"MODIS-13H", "13H", image});
                    else if (i == 7)
                        modis_products.images.push_back({"MODIS-14L", "14L", image});
                    else if (i == 8)
                        modis_products.images.push_back({"MODIS-14H", "14H", image});
                    else
                        modis_products.images.push_back({"MODIS-" + std::to_string(i + 6), std::to_string(i + 6), image});
                }

                // Calibration
                nlohmann::json calib_cfg;
                calib_cfg["calibrator"] = "eos_modis";
                calib_cfg["vars"] = modis::precompute::precomputeVars(&modis_products, modis_reader.getCalib(), d_satellite == AQUA);
                calib_cfg["is_aqua"] = d_satellite == AQUA;
                calib_cfg["bowtie_lut_1km"] = bowtie_lut_1km;
                modis_products.set_calibration(calib_cfg);
                for (int i = 0; i < 21; i++)
                    modis_products.set_calibration_type(i, satdump::ImageProducts::CALIB_REFLECTANCE);
                for (int i = 21; i < 38; i++)
                    modis_products.set_calibration_type(i, satdump::ImageProducts::CALIB_RADIANCE);
                modis_products.set_calibration_type(27, satdump::ImageProducts::CALIB_REFLECTANCE);

                for (int i = 0; i < 38; i++)
                { // Set to 0 for now
                    modis_products.set_wavenumber(i, -1);
                    modis_products.set_calibration_default_radiance_range(i, 0, 0);
                }

                auto modis_table = loadJsonFile(resources::getResourcePath("calibration/modis_table.json"));

                for (int i = 0; i < 16; i++)
                {
                    int ch = i;
                    if (ch >= 6)
                        ch++;
                    modis_products.set_wavenumber(21 + ch, freq_to_wavenumber(299792458.0 / modis_table["wavelengths"][i].get<double>()));
                    modis_products.set_calibration_default_radiance_range(21 + ch, modis_table["default_ranges"][i][0].get<double>(), modis_table["default_ranges"][i][1].get<double>());
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
                airs_hd_products.set_timestamps(airs_reader.timestamps_ifov);
                airs_hd_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/aqua_airs.json")));

                for (int i = 0; i < 4; i++)
                    airs_hd_products.images.push_back({"AIRS-HD-" + std::to_string(i + 1), std::to_string(i + 1), airs_reader.getHDChannel(i)});

                airs_hd_products.save(directory_hd);
                dataset.products_list.push_back("AIRS/HD");

                satdump::ImageProducts airs_products;
                airs_products.instrument_name = "airs";
                airs_products.has_timestamps = true;
                airs_products.bit_depth = 16;
                airs_products.save_as_matrix = true;
                airs_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_IFOV;
                airs_products.set_tle(satellite_tle);
                airs_products.set_timestamps(airs_reader.timestamps_ifov);
                airs_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/aqua_airs.json")));

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

                {
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CERES-FM3";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directory(directory);

                    satdump::ImageProducts ceres_products;
                    ceres_products.instrument_name = "ceres";
                    ceres_products.has_timestamps = true;
                    ceres_products.set_tle(satellite_tle);
                    ceres_products.bit_depth = 16;
                    ceres_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                    ceres_products.set_timestamps(ceres_fm3_reader.timestamps);
                    ceres_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/aqua_ceres_fm3.json")));

                    ceres_products.images.push_back({"CERES-LONGWAVE", "l", ceres_fm3_reader.getImage(0)});
                    ceres_products.images.push_back({"CERES-SHORTWAVE", "s", ceres_fm3_reader.getImage(1)});
                    ceres_products.images.push_back({"CERES-TOTAL", "t", ceres_fm3_reader.getImage(2)});

                    ceres_products.save(directory);
                    dataset.products_list.push_back("CERES-FM3");
                    ceres_fm3_status = DONE;
                }

                {
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CERES-FM4";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directory(directory);

                    satdump::ImageProducts ceres_products;
                    ceres_products.instrument_name = "ceres";
                    ceres_products.has_timestamps = true;
                    ceres_products.set_tle(satellite_tle);
                    ceres_products.bit_depth = 16;
                    ceres_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                    ceres_products.set_timestamps(ceres_fm4_reader.timestamps);
                    ceres_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/aqua_ceres_fm4.json")));

                    ceres_products.images.push_back({"CERES-LONGWAVE", "l", ceres_fm4_reader.getImage(0)});
                    ceres_products.images.push_back({"CERES-SHORTWAVE", "s", ceres_fm4_reader.getImage(1)});
                    ceres_products.images.push_back({"CERES-TOTAL", "t", ceres_fm4_reader.getImage(2)});

                    ceres_products.save(directory);
                    dataset.products_list.push_back("CERES-FM4");
                    ceres_fm3_status = DONE;
                }
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

                auto img = omi_1_reader.getImageRaw();
                image::save_img(img, directory + "/OMI-1");
                img = omi_2_reader.getImageRaw();
                image::save_img(img, directory + "/OMI-2");

                img = omi_1_reader.getImageVisible();
                image::save_img(img, directory + "/OMI-VIS-1");
                img = omi_2_reader.getImageVisible();
                image::save_img(img, directory + "/OMI-VIS-2");

                image::Image imageAll1 = image::make_manyimg_composite(33, 24, 792, [this](int c)
                                                                       { return omi_1_reader.getChannel(c); });
                image::Image imageAll2 = image::make_manyimg_composite(33, 24, 792, [this](int c)
                                                                       { return omi_2_reader.getChannel(c); });
                image::save_img(imageAll1, directory + "/OMI-ALL-1");
                image::save_img(imageAll2, directory + "/OMI-ALL-2");
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
                    ImGui::TextColored(style::theme.green, "%d", modis_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(modis_status);
                }

                if (d_satellite == AQUA)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("AIRS");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", airs_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(airs_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("AMSU A1");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", amsu_a1_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(amsu_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("AMSU A2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", amsu_a2_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(amsu_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("CERES FM-3");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", ceres_fm3_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(ceres_fm3_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("CERES FM-4");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", ceres_fm4_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(ceres_fm4_status);
                }

                if (d_satellite == AURA)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("OMI 1");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", omi_1_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(omi_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("OMI 2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", omi_2_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(omi_status);
                }

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

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