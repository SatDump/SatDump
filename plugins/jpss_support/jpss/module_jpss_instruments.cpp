#include "module_jpss_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_weather/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "jpss.h"
#include "common/image/bowtie.h"
#include "common/ccsds/ccsds_weather/demuxer.h"
#include "products/products.h"
#include "products/image_products.h"
#include "products/dataset.h"
#include "resources.h"

namespace jpss
{
    namespace instruments
    {
        JPSSInstrumentsDecoderModule::JPSSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters),
              npp_mode(parameters["npp_mode"].get<bool>())
        {
        }

        void JPSSInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1279]; // Oversized for NPP, but not a big deal

            int mpdu_size = npp_mode ? 884 : 1094;
            int insert_zone_size = npp_mode ? 0 : 9;

            // Demuxers
            ccsds::ccsds_weather::Demuxer demuxer_vcid1(mpdu_size, true, insert_zone_size);
            ccsds::ccsds_weather::Demuxer demuxer_vcid6(mpdu_size, true, insert_zone_size);
            ccsds::ccsds_weather::Demuxer demuxer_vcid11(mpdu_size, true, insert_zone_size);
            ccsds::ccsds_weather::Demuxer demuxer_vcid16(mpdu_size, true, insert_zone_size);

            std::vector<uint8_t> jpss_scids;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, npp_mode ? 1024 : 1279);

                // Parse this transport frame
                ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

                if (vcdu.spacecraft_id == SNPP_SCID ||
                    vcdu.spacecraft_id == JPSS1_SCID ||
                    vcdu.spacecraft_id == JPSS2_SCID /*||
                    vcdu.spacecraft_id == JPSS3_SCID ||
                    vcdu.spacecraft_id == JPSS4_SCID*/
                )
                    jpss_scids.push_back(vcdu.spacecraft_id);

                if (vcdu.vcid == 1) // ATMS
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 528)
                            atms_reader.work(pkt);
                }
                else if (vcdu.vcid == 6) // CrIS
                {
                    // std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid6.work(cadu);
                    // for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    //     if (pkt.header.apid == 1340)
                    //     {
                    //         logger->info(pkt.payload.size() + 6);
                    //     }
                }
                else if (vcdu.vcid == 11) // OMPS
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid11.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 616 || pkt.header.apid == 560)
                            omps_nadir_reader.work(pkt);
                        else if (pkt.header.apid == 617 || pkt.header.apid == 561)
                            omps_limb_reader.work(pkt);
                }
                else if (vcdu.vcid == 16) // VIIRS
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid16.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        /*
                        Feeding all readers directly that way certainly is
                        not the absolute best way, but since APID selection
                        could change, it doesn't really impact anything that
                        much anyway.
                        */

                        // Moderate resolution channels
                        for (int i = 0; i < 16; i++)
                            viirs_reader_moderate[i].feed(pkt);

                        // Imaging channels
                        for (int i = 0; i < 5; i++)
                            viirs_reader_imaging[i].feed(pkt);

                        // DNB channels
                        for (int i = 0; i < 3; i++)
                            viirs_reader_dnb[i].feed(pkt);
                    }
                }

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%%");
                }
            }

            data_in.close();

            int scid = most_common(jpss_scids.begin(), jpss_scids.end());
            jpss_scids.clear();

            std::string sat_name = "Unknown JPSS";
            if (scid == SNPP_SCID)
                sat_name = "Suomi NPP";
            else if (scid == JPSS1_SCID)
                sat_name = "NOAA 20 (JPSS-1)";
            else if (scid == JPSS2_SCID)
                sat_name = "NOAA 21 (JPSS-2)";

            int norad = 0;
            if (scid == SNPP_SCID)
                norad = SNPP_NORAD;
            else if (scid == JPSS1_SCID)
                norad = JPSS1_NORAD;
            else if (scid == JPSS2_SCID)
                norad = JPSS2_NORAD;
            else if (scid == JPSS3_SCID)
                norad = JPSS3_NORAD;
            else if (scid == JPSS4_SCID)
                norad = JPSS4_NORAD;

            std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry.get_from_norad(norad);

            // Products dataset
            satdump::ProductDataSet dataset;
            dataset.satellite_name = sat_name;
            dataset.timestamp = avg_overflowless(atms_reader.timestamps);

            // Satellite ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : " + sat_name);
            }

            // ATMS
            {
                atms_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ATMS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- ATMS");
                logger->info("Lines : " + std::to_string(atms_reader.lines));

                satdump::ImageProducts atms_products;
                atms_products.instrument_name = "atms";
                atms_products.has_timestamps = true;
                atms_products.set_tle(satellite_tle);
                atms_products.bit_depth = 16;
                atms_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                atms_products.set_timestamps(atms_reader.timestamps);
                atms_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/jpss_atms.json")));

                for (int i = 0; i < 22; i++)
                    atms_products.images.push_back({"ATMS-" + std::to_string(i + 1), std::to_string(i + 1), atms_reader.getChannel(i)});

                atms_products.save(directory);
                dataset.products_list.push_back("ATMS");

                atms_status = DONE;
            }

            // OMPS NADIR
            {
                omps_nadir_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/OMPS/Nadir";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directories(directory);

                logger->info("----------- OMPS-NADIR");
                logger->info("Lines : " + std::to_string(omps_nadir_reader.lines));

                for (int i = 0; i < 339; i++)
                {
                    WRITE_IMAGE(omps_nadir_reader.getChannel(i), directory + "/OMPS-NADIR-" + std::to_string(i + 1));
                }
                omps_nadir_status = DONE;
            }

            // OMPS LIMB
            {
                omps_limb_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/OMPS/Limb";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directories(directory);

                logger->info("----------- OMPS-NADIR");
                logger->info("Lines : " + std::to_string(omps_nadir_reader.lines));

                for (int i = 0; i < 135; i++)
                {
                    WRITE_IMAGE(omps_limb_reader.getChannel(i), directory + "/OMPS-LIMB-" + std::to_string(i + 1));
                }
                omps_limb_status = DONE;
            }

            // VIIRS
            {
                viirs_dnb_status = PROCESSING;
                for (int i = 0; i < 16; i++)
                {
                    viirs_moderate_status[i] = PROCESSING;
                    if (i < 5)
                        viirs_imaging_status[i] = PROCESSING;
                }

                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/VIIRS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directories(directory);

                logger->info("----------- VIIRS");
                for (int i = 0; i < 16; i++)
                    logger->info("M" + std::to_string(i + 1) + " Segments : " + std::to_string(viirs_reader_moderate[i].segments.size()));
                for (int i = 0; i < 5; i++)
                    logger->info("I" + std::to_string(i + 1) + " Segments : " + std::to_string(viirs_reader_imaging[i].segments.size()));
                logger->info("DNB Segments : " + std::to_string(viirs_reader_dnb[0].segments.size()));

                process_viirs_channels(); // Differential decoding!

                // BowTie values
                const float alpha = 1.0 / 1.9;
                const float beta = 0.52333; // 1.0 - alpha;

                satdump::ImageProducts viirs_products;
                viirs_products.instrument_name = "viirs";
                viirs_products.has_timestamps = true;
                viirs_products.needs_correlation = true;
                viirs_products.set_tle(satellite_tle);
                viirs_products.bit_depth = 16;
                viirs_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_MULTIPLE_LINES;
                viirs_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/jpss_viirs.json")));

                for (int i = 0; i < 16; i++)
                {
                    if (viirs_reader_moderate[i].segments.size() > 0)
                    {
                        logger->info("M" + std::to_string(i + 1) + "...");
                        image::Image<uint16_t> viirs_image = viirs_reader_moderate[i].getImage();
                        viirs_image = image::bowtie::correctGenericBowTie(viirs_image, 1, viirs_reader_moderate[i].channelSettings.zoneHeight, alpha, beta);
                        viirs_moderate_status[i] = SAVING;

                        viirs_products.images.push_back({"VIIRS-M" + std::to_string(i + 1),
                                                         "m" + std::to_string(i + 1),
                                                         viirs_image,
                                                         viirs_reader_moderate[i].timestamps,
                                                         viirs_reader_moderate[i].channelSettings.zoneHeight});
                    }
                    viirs_moderate_status[i] = DONE;
                }

                for (int i = 0; i < 5; i++)
                {
                    if (viirs_reader_imaging[i].segments.size() > 0)
                    {
                        logger->info("I" + std::to_string(i + 1) + "...");
                        image::Image<uint16_t> viirs_image = viirs_reader_imaging[i].getImage();
                        viirs_image = image::bowtie::correctGenericBowTie(viirs_image, 1, viirs_reader_imaging[i].channelSettings.zoneHeight, alpha, beta);
                        viirs_imaging_status[i] = SAVING;

                        viirs_products.images.push_back({"VIIRS-I" + std::to_string(i + 1),
                                                         "i" + std::to_string(i + 1),
                                                         viirs_image,
                                                         viirs_reader_imaging[i].timestamps,
                                                         viirs_reader_imaging[i].channelSettings.zoneHeight});
                    }
                    viirs_imaging_status[i] = DONE;
                }

                viirs_dnb_status = SAVING;
                if (viirs_reader_dnb[0].segments.size() > 0)
                {
                    logger->info("DNB...");

                    viirs_products.images.push_back({"VIIRS-DNB",
                                                     "dnb",
                                                     viirs_reader_dnb[0].getImage(),
                                                     viirs_reader_dnb[0].timestamps,
                                                     viirs_reader_dnb[0].channelSettings.zoneHeight});
                }

                if (viirs_reader_dnb[1].segments.size() > 0)
                {
                    logger->info("DNB MGS...");

                    viirs_products.images.push_back({"VIIRS-DNB-MGS",
                                                     "dnbmgs",
                                                     viirs_reader_dnb[1].getImage(),
                                                     viirs_reader_dnb[1].timestamps,
                                                     viirs_reader_dnb[1].channelSettings.zoneHeight});
                }

                if (viirs_reader_dnb[2].segments.size() > 0)
                {
                    logger->info("DNB LGS...");

                    viirs_products.images.push_back({"VIIRS-DNB-LGS",
                                                     "dnblgs",
                                                     viirs_reader_dnb[2].getImage(),
                                                     viirs_reader_dnb[2].timestamps,
                                                     viirs_reader_dnb[2].channelSettings.zoneHeight});
                }
                viirs_dnb_status = DONE;

                viirs_products.save(directory);
                dataset.products_list.push_back("VIIRS");
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void JPSSInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("JPSS Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##jpssinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
                ImGui::Text("ATMS");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", atms_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(atms_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("OMPS Nadir");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", omps_nadir_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(omps_nadir_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("OMPS Limb");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", omps_limb_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(omps_limb_status);

                for (int i = 0; i < 16; i++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("VIIRS M%d", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", (int)viirs_reader_moderate[i].segments.size());
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(viirs_moderate_status[i]);
                }

                for (int i = 0; i < 5; i++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("VIIRS I%d", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", (int)viirs_reader_imaging[i].segments.size());
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(viirs_imaging_status[i]);
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("VIIRS DNB");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", (int)viirs_reader_dnb[0].segments.size());
                ImGui::TableSetColumnIndex(2);
                drawStatus(viirs_dnb_status);

                ImGui::EndTable();
            }

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        void JPSSInstrumentsDecoderModule::process_viirs_channels()
        {
            // Differential decoding for M5, M3, M2, M1
            logger->info("Diff M5...");
            viirs_reader_moderate[5 - 1].differentialDecode(viirs_reader_moderate[4 - 1], 1);
            logger->info("Diff M3...");
            viirs_reader_moderate[3 - 1].differentialDecode(viirs_reader_moderate[4 - 1], 1);
            logger->info("Diff M2...");
            viirs_reader_moderate[2 - 1].differentialDecode(viirs_reader_moderate[3 - 1], 1);
            logger->info("Diff M1...");
            viirs_reader_moderate[1 - 1].differentialDecode(viirs_reader_moderate[2 - 1], 1);

            // Differential decoding for M8, M11
            logger->info("Diff M8...");
            viirs_reader_moderate[8 - 1].differentialDecode(viirs_reader_moderate[10 - 1], 1);
            logger->info("Diff M11...");
            viirs_reader_moderate[11 - 1].differentialDecode(viirs_reader_moderate[10 - 1], 1);

            // Differential decoding for M14
            logger->info("Diff M14...");
            viirs_reader_moderate[14 - 1].differentialDecode(viirs_reader_moderate[15 - 1], 1);

            // Differential decoding for I2, I3
            logger->info("Diff I2...");
            viirs_reader_imaging[2 - 1].differentialDecode(viirs_reader_imaging[1 - 1], 1);

            logger->info("Diff I3...");
            viirs_reader_imaging[3 - 1].differentialDecode(viirs_reader_imaging[2 - 1], 1);

            // Differential decoding for I4 and I5
            logger->info("Diff I4...");
            viirs_reader_imaging[4 - 1].differentialDecode(viirs_reader_moderate[12 - 1], 2);

            logger->info("Diff I5...");
            viirs_reader_imaging[5 - 1].differentialDecode(viirs_reader_moderate[15 - 1], 2);
        }

        std::string JPSSInstrumentsDecoderModule::getID()
        {
            return "jpss_instruments";
        }

        std::vector<std::string> JPSSInstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> JPSSInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<JPSSInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop