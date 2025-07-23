#include "module_eos_instruments.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/utils.h"
#include "core/resources.h"
#include "image/bowtie.h"
#include "imgui/imgui.h"
#include "instruments/modis/modis_histmatch.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "common/calibration.h"
#include "core/exception.h"
#include "instruments/modis/calibrator/modis_calibrator.h"

#include "common/tracking/tle.h"
#include "image/image_utils.h"
#include "image/io.h"

namespace eos
{
    namespace instruments
    {
        EOSInstrumentsDecoderModule::EOSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), d_modis_bowtie(d_parameters["modis_bowtie"].get<bool>())
        {
            if (parameters["satellite"] == "terra")
                d_satellite = TERRA;
            else if (parameters["satellite"] == "aqua")
                d_satellite = AQUA;
            else if (parameters["satellite"] == "aura")
                d_satellite = AURA;
            else
                throw satdump_exception("EOS Instruments Decoder : EOS satellite \"" + parameters["satellite"].get<std::string>() + "\" is not valid!");
            fsfsm_enable_output = false;
        }

        void EOSInstrumentsDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MODIS";

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

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1024);

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
            }

            cleanup();

            // Products dataset
            satdump::products::DataSet dataset;
            if (d_satellite == AQUA)
                dataset.satellite_name = "Aqua";
            else if (d_satellite == TERRA)
                dataset.satellite_name = "Terra";
            else if (d_satellite == AURA)
                dataset.satellite_name = "Aura";

            if (d_satellite == AQUA || d_satellite == TERRA) // MODIS
                dataset.timestamp = satdump::get_median(modis_reader.timestamps_1000);
            else
                dataset.timestamp = time(0);

            std::optional<satdump::TLE> satellite_tle;
            if (d_satellite == AQUA)
                satellite_tle = satdump::general_tle_registry->get_from_norad_time(27424, dataset.timestamp);
            else if (d_satellite == TERRA)
                satellite_tle = satdump::general_tle_registry->get_from_norad_time(25994, dataset.timestamp);
            else if (d_satellite == AURA)
                satellite_tle = satdump::general_tle_registry->get_from_norad_time(28376, dataset.timestamp);

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

                satdump::products::ImageProduct modis_products;
                modis_products.instrument_name = "modis";
                nlohmann::json proj_cfg;
                if (d_satellite == AQUA)
                    proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/aqua_modis.json"));
                else if (d_satellite == TERRA)
                    proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/terra_modis.json"));
                // if (d_satellite == AQUA)
                //     proj_cfg["ephemeris"] = gbad_reader.getEphem();
                modis_products.set_proj_cfg_tle_timestamps(proj_cfg, satellite_tle, modis_reader.timestamps_250);

                for (int i = 0; i < 2; i++)
                {
                    image::Image image = modis_reader.getImage250m(i);
                    // modis::modis_match_detector_histograms(image, 1 /*4*/, 40 * 2);
                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_250, alpha, beta);
                    modis_products.images.push_back({i, "MODIS-" + std::to_string(i + 1), std::to_string(i + 1), image, 12});
                }

                for (int i = 0; i < 5; i++)
                {
                    image::Image image = modis_reader.getImage500m(i);
                    // modis::modis_match_detector_histograms(image, 1 /*2*/, 20 * 2);
                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_500, alpha, beta);
                    modis_products.images.push_back({i + 2, "MODIS-" + std::to_string(i + 3), std::to_string(i + 3), image, 12, satdump::ChannelTransform().init_affine(2, 2, 0, 0)});
                }

                std::vector<std::vector<int>> bowtie_lut_1km;
                for (int i = 0; i < 31; i++)
                {
                    image::Image image = modis_reader.getImage1000m(i);

                    // modis::modis_match_detector_histograms(image, 1, 10 * 2);

                    if (d_modis_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_1000, alpha, beta, &bowtie_lut_1km);

                    auto cht = satdump::ChannelTransform().init_affine(4, 4, 0, 0);

                    if (i < 5)
                        modis_products.images.push_back({i + 7, "MODIS-" + std::to_string(i + 8), std::to_string(i + 8), image, 12, cht});
                    else if (i == 5)
                        modis_products.images.push_back({i + 7, "MODIS-13L", "13L", image, 12, cht});
                    else if (i == 6)
                        modis_products.images.push_back({i + 7, "MODIS-13H", "13H", image, 12, cht});
                    else if (i == 7)
                        modis_products.images.push_back({i + 7, "MODIS-14L", "14L", image, 12, cht});
                    else if (i == 8)
                        modis_products.images.push_back({i + 7, "MODIS-14H", "14H", image, 12, cht});
                    else
                        modis_products.images.push_back({i + 7, "MODIS-" + std::to_string(i + 6), std::to_string(i + 6), image, 12, cht});
                }

                // Calibration
                nlohmann::json calib_cfg;
                calib_cfg["vars"] = modis::precompute::precomputeVars(&modis_products, modis_reader.getCalib(), d_satellite == AQUA);
                calib_cfg["is_aqua"] = d_satellite == AQUA;
                calib_cfg["bowtie_lut_1km"] = bowtie_lut_1km;
                modis_products.set_calibration("eos_modis", calib_cfg);
                for (int i = 0; i < 21; i++)
                    modis_products.set_channel_unit(i, CALIBRATION_ID_REFLECTIVE_RADIANCE);
                for (int i = 21; i < 38; i++)
                    modis_products.set_channel_unit(i, CALIBRATION_ID_EMISSIVE_RADIANCE);
                modis_products.set_channel_unit(27, CALIBRATION_ID_REFLECTIVE_RADIANCE);

                // for (int i = 0; i < 38; i++)
                // { // Set to 0 for now
                //     modis_products.set_channel_wavenumber(i, -1);
                // }

                auto modis_table = loadJsonFile(resources::getResourcePath("calibration/modis_table.json"));

                for (int i = 0; i < 16; i++)
                {
                    int ch = i;
                    if (ch >= 6)
                        ch++;
                    modis_products.set_channel_wavenumber(21 + ch, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / modis_table["wavelengths"][i].get<double>()));
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

                satdump::products::ImageProduct airs_hd_products;
                airs_hd_products.instrument_name = "airs_hd";
                airs_hd_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/aqua_airs.json")), satellite_tle, airs_reader.timestamps_ifov);

                for (int i = 0; i < 4; i++)
                    airs_hd_products.images.push_back({i, "AIRS-HD-" + std::to_string(i + 1), std::to_string(i + 1), airs_reader.getHDChannel(i), 16});

                airs_hd_products.save(directory_hd);
                dataset.products_list.push_back("AIRS/HD");

                satdump::products::ImageProduct airs_products;
                airs_products.instrument_name = "airs";
                airs_products.save_as_matrix = true;
                airs_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/aqua_airs.json")), satellite_tle, airs_reader.timestamps_ifov);

                for (int i = 0; i < 2666; i++)
                    airs_products.images.push_back({i, "AIRS-" + std::to_string(i + 1), std::to_string(i + 1), airs_reader.getChannel(i), 16});

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

                satdump::products::ImageProduct amsu_products;
                amsu_products.instrument_name = "amsu_a";
                amsu_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/aqua_amsu.json")), satellite_tle, amsu_a1_reader.timestamps);
                // TODOREWORK CORRELATE AMSU
                logger->critical(" TODOREWORK CORRELATE AMSU. If you see this, PLEASE REPORT");

                for (int i = 0; i < 2; i++)
                    amsu_products.images.push_back({i, "AMSU-A2-" + std::to_string(i + 1), std::to_string(i + 1), amsu_a2_reader.getChannel(i), 16});

                for (int i = 0; i < 13; i++)
                    amsu_products.images.push_back({i + 2, "AMSU-A1-" + std::to_string(i + 1), std::to_string(i + 3), amsu_a1_reader.getChannel(i), 16});

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

                    satdump::products::ImageProduct ceres_products;
                    ceres_products.instrument_name = "ceres";
                    ceres_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/aqua_ceres_fm3.json")), satellite_tle, ceres_fm3_reader.timestamps);

                    ceres_products.images.push_back({0, "CERES-LONGWAVE", "l", ceres_fm3_reader.getImage(0), 16});
                    ceres_products.images.push_back({1, "CERES-SHORTWAVE", "s", ceres_fm3_reader.getImage(1), 16});
                    ceres_products.images.push_back({2, "CERES-TOTAL", "t", ceres_fm3_reader.getImage(2), 16});

                    ceres_products.save(directory);
                    dataset.products_list.push_back("CERES-FM3");
                    ceres_fm3_status = DONE;
                }

                {
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CERES-FM4";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directory(directory);

                    satdump::products::ImageProduct ceres_products;
                    ceres_products.instrument_name = "ceres";
                    ceres_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/aqua_ceres_fm4.json")), satellite_tle, ceres_fm4_reader.timestamps);

                    ceres_products.images.push_back({0, "CERES-LONGWAVE", "l", ceres_fm4_reader.getImage(0), 16});
                    ceres_products.images.push_back({1, "CERES-SHORTWAVE", "s", ceres_fm4_reader.getImage(1), 16});
                    ceres_products.images.push_back({2, "CERES-TOTAL", "t", ceres_fm4_reader.getImage(2), 16});

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

                image::Image imageAll1 = image::make_manyimg_composite(33, 24, 792, [this](int c) { return omi_1_reader.getChannel(c); });
                image::Image imageAll2 = image::make_manyimg_composite(33, 24, 792, [this](int c) { return omi_2_reader.getChannel(c); });
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

            drawProgressBar();

            ImGui::End();
        }

        std::string EOSInstrumentsDecoderModule::getID() { return "eos_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> EOSInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<EOSInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace eos