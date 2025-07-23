#include "module_jpss_instruments.h"
#include "common/calibration.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/utils.h"
#include "core/resources.h"
#include "image/bowtie.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "instruments/viirs/viirs_viewang.h"
#include "jpss.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "common/tracking/tle.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include "utils/stats.h"

namespace jpss
{
    namespace instruments
    {
        JPSSInstrumentsDecoderModule::JPSSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), npp_mode(parameters["npp_mode"].get<bool>())
        {
            fsfsm_enable_output = false;
        }

        void JPSSInstrumentsDecoderModule::process()
        {

            uint8_t cadu[1279]; // Oversized for NPP, but not a big deal

            int mpdu_size = npp_mode ? 884 : 1094;
            int insert_zone_size = npp_mode ? 0 : 9;

            // Demuxers
            ccsds::ccsds_aos::Demuxer demuxer_vcid0(mpdu_size, true, insert_zone_size);
            ccsds::ccsds_aos::Demuxer demuxer_vcid1(mpdu_size, true, insert_zone_size);
            ccsds::ccsds_aos::Demuxer demuxer_vcid6(mpdu_size, true, insert_zone_size);
            ccsds::ccsds_aos::Demuxer demuxer_vcid11(mpdu_size, true, insert_zone_size);
            ccsds::ccsds_aos::Demuxer demuxer_vcid16(mpdu_size, true, insert_zone_size);

            std::vector<uint8_t> jpss_scids;

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, npp_mode ? 1024 : 1279);

                // Parse this transport frame
                ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

                if (vcdu.spacecraft_id == SNPP_SCID || vcdu.spacecraft_id == JPSS1_SCID || vcdu.spacecraft_id == JPSS2_SCID /*||
                                                                                           vcdu.spacecraft_id == JPSS3_SCID ||
                                                                                           vcdu.spacecraft_id == JPSS4_SCID*/
                )
                    jpss_scids.push_back(vcdu.spacecraft_id);

                if (vcdu.vcid == 0) // HK/TM
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid0.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 11)
                            att_ephem.work(pkt);
                }
                else if (vcdu.vcid == 1) // ATMS
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 528)
                            atms_reader.work(pkt);
                        else if (pkt.header.apid == 515)
                            atms_reader.work_calib(pkt);
                        else if (pkt.header.apid == 530)
                            atms_reader.work_hotcal(pkt);
                        else if (pkt.header.apid == 531)
                            atms_reader.work_eng(pkt);
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
            }

            cleanup();

            int scid = satdump::most_common(jpss_scids.begin(), jpss_scids.end(), 0);
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

            // Products dataset
            satdump::products::DataSet dataset;
            dataset.satellite_name = sat_name;
            dataset.timestamp = satdump::get_median(atms_reader.timestamps);

            std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry->get_from_norad_time(norad, dataset.timestamp);

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

                satdump::products::ImageProduct atms_products;
                atms_products.instrument_name = "atms";
                auto proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/jpss_atms.json"));
                if (d_parameters["use_ephemeris"].get<bool>())
                    proj_cfg["ephemeris"] = att_ephem.getEphem();
                atms_products.set_proj_cfg_tle_timestamps(proj_cfg, satellite_tle, atms_reader.timestamps);

                for (int i = 0; i < 22; i++)
                    atms_products.images.push_back({i, "ATMS-" + std::to_string(i + 1), std::to_string(i + 1), atms_reader.getChannel(i), 16});

                nlohmann::json calib_coefs = loadCborFile(resources::getResourcePath("calibration/ATMS.cbor"));
                if (calib_coefs.contains(sat_name))
                {
                    atms::ATMS_SDR_CC sdr_cc = calib_coefs[sat_name];
                    nlohmann::json calib_cfg;
                    calib_cfg["vars"] = atms_reader.getCalib();
                    calib_cfg["sdr_cc"] = calib_coefs[sat_name];
                    atms_products.set_calibration("jpss_atms", calib_cfg);
                    for (int c = 0; c < 22; c++)
                    {
                        atms_products.set_channel_unit(c, CALIBRATION_ID_EMISSIVE_RADIANCE);
                        atms_products.set_channel_frequency(c, sdr_cc.centralFrequency[c]);
                    }
                }
                else
                    logger->warn("(ATMS) Calibration data for " + sat_name + " not found. Calibration will not be performed");

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
                    auto img = omps_nadir_reader.getChannel(i);
                    image::save_img(img, directory + "/OMPS-NADIR-" + std::to_string(i + 1));
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
                    auto img = omps_limb_reader.getChannel(i);
                    image::save_img(img, directory + "/OMPS-LIMB-" + std::to_string(i + 1));
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
                std::string directory_dnb = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/VIIRS-DNB";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directories(directory);
                if (!std::filesystem::exists(directory_dnb))
                    std::filesystem::create_directories(directory_dnb);

                logger->info("----------- VIIRS");
                for (int i = 0; i < 5; i++)
                    logger->info("I" + std::to_string(i + 1) + " Segments : " + std::to_string(viirs_reader_imaging[i].segments.size()));
                for (int i = 0; i < 16; i++)
                    logger->info("M" + std::to_string(i + 1) + " Segments : " + std::to_string(viirs_reader_moderate[i].segments.size()));
                logger->info("DNB Segments : " + std::to_string(viirs_reader_dnb[0].segments.size()));

                process_viirs_channels(); // Differential decoding!

                // BowTie values
                const float alpha = 1.0 / 1.9;
                const float beta = 0.52333; // 1.0 - alpha;

                // Normal channels
                satdump::products::ImageProduct viirs_products;
                viirs_products.instrument_name = "viirs";

                nlohmann::json proj_cfg_all;
                if (scid == SNPP_SCID)
                    proj_cfg_all = loadJsonFile(resources::getResourcePath("projections_settings/npp_viirs.json"));
                else if (scid == JPSS1_SCID)
                    proj_cfg_all = loadJsonFile(resources::getResourcePath("projections_settings/jpss1_viirs.json"));
                else if (scid == JPSS2_SCID)
                    proj_cfg_all = loadJsonFile(resources::getResourcePath("projections_settings/jpss2_viirs.json"));

                auto proj_cfg = proj_cfg_all["proj"];
                auto norm_cfg = proj_cfg_all["normal"];
                auto dnb_cfg = proj_cfg_all["dnb"];

                if (d_parameters["use_ephemeris"].get<bool>())
                    proj_cfg["ephemeris"] = att_ephem.getEphem();
                viirs_products.set_proj_cfg_tle_timestamps(proj_cfg, satellite_tle, viirs_reader_imaging[0].timestamps); // All the same now. Merge to a single "timestamps"?

                auto points_viirs_normal_ch =
                    viirs::calculateVIIRSViewAnglePoints(false, getValueOrDefault(norm_cfg["is_n20"], false), norm_cfg["scan_angle"], getValueOrDefault(norm_cfg["roll_offset"], 0.0));
                auto points_viirs_dnb_ch =
                    viirs::calculateVIIRSViewAnglePoints(true, getValueOrDefault(dnb_cfg["is_n20"], false), dnb_cfg["scan_angle"], getValueOrDefault(dnb_cfg["roll_offset"], 0.0));

                for (int i = 0; i < 5; i++)
                {
                    if (viirs_reader_imaging[i].segments.size() > 0)
                    {
                        logger->info("I" + std::to_string(i + 1) + "...");
                        image::Image viirs_image = viirs_reader_imaging[i].getImage();
                        viirs_image = image::bowtie::correctGenericBowTie(viirs_image, 1, viirs_reader_imaging[i].channelSettings.zoneHeight, alpha, beta);
                        viirs_imaging_status[i] = SAVING;

                        viirs_products.images.push_back(
                            {i, "VIIRS-I" + std::to_string(i + 1), "i" + std::to_string(i + 1), viirs_image, 16, satdump::ChannelTransform().init_affine_interpx(1, 1, 0, 0, points_viirs_normal_ch)});
                    }
                    viirs_imaging_status[i] = DONE;
                }

                for (int i = 0; i < 16; i++)
                {
                    if (viirs_reader_moderate[i].segments.size() > 0)
                    {
                        logger->info("M" + std::to_string(i + 1) + "...");
                        image::Image viirs_image = viirs_reader_moderate[i].getImage();
                        viirs_image = image::bowtie::correctGenericBowTie(viirs_image, 1, viirs_reader_moderate[i].channelSettings.zoneHeight, alpha, beta);
                        viirs_moderate_status[i] = SAVING;

                        viirs_products.images.push_back({i + 5, "VIIRS-M" + std::to_string(i + 1), "m" + std::to_string(i + 1), viirs_image, 16,
                                                         satdump::ChannelTransform().init_affine_interpx(2, 2, 0, 0, points_viirs_normal_ch)});
                    }
                    viirs_moderate_status[i] = DONE;
                }

                viirs_dnb_status = SAVING;
                if (viirs_reader_dnb[0].segments.size() > 0)
                {
                    logger->info("DNB...");

                    viirs_products.images.push_back({21, "VIIRS-DNB", "dnb", viirs_reader_dnb[0].getImage(), 16, satdump::ChannelTransform().init_affine_interpx(1, 2, 0, 1, points_viirs_dnb_ch)});
                }

                if (viirs_reader_dnb[1].segments.size() > 0)
                {
                    logger->info("DNB MGS...");

                    viirs_products.images.push_back(
                        {22, "VIIRS-DNB-MGS", "dnbmgs", viirs_reader_dnb[1].getImage(), 16, satdump::ChannelTransform().init_affine_interpx(1, 2, 0, 1, points_viirs_dnb_ch)});
                }

                if (viirs_reader_dnb[2].segments.size() > 0)
                {
                    logger->info("DNB LGS...");

                    viirs_products.images.push_back(
                        {23, "VIIRS-DNB-LGS", "dnblgs", viirs_reader_dnb[2].getImage(), 16, satdump::ChannelTransform().init_affine_interpx(1, 2, 0, 1, points_viirs_dnb_ch)});
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
                ImGui::TextColored(style::theme.green, "%d", atms_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(atms_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("OMPS Nadir");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", omps_nadir_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(omps_nadir_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("OMPS Limb");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", omps_limb_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(omps_limb_status);

                for (int i = 0; i < 16; i++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("VIIRS M%d", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", (int)viirs_reader_moderate[i].segments.size());
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(viirs_moderate_status[i]);
                }

                for (int i = 0; i < 5; i++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("VIIRS I%d", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", (int)viirs_reader_imaging[i].segments.size());
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(viirs_imaging_status[i]);
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("VIIRS DNB");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", (int)viirs_reader_dnb[0].segments.size());
                ImGui::TableSetColumnIndex(2);
                drawStatus(viirs_dnb_status);

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        inline bool vectorContains(std::vector<double> v, double f)
        {
            for (auto &c : v)
                if (c == f)
                    return true;
            return false;
        }

        void JPSSInstrumentsDecoderModule::process_viirs_channels()
        {
            {
                std::vector<double> timestamps;
                for (int i = 0; i < 5; i++)
                    for (auto &seg : viirs_reader_imaging[i].segments)
                        if (!vectorContains(timestamps, seg.timestamp))
                            timestamps.push_back(seg.timestamp);
                for (int i = 0; i < 16; i++)
                    for (auto &seg : viirs_reader_moderate[i].segments)
                        if (!vectorContains(timestamps, seg.timestamp))
                            timestamps.push_back(seg.timestamp);
                for (int i = 0; i < 3; i++)
                    for (auto &seg : viirs_reader_dnb[i].segments)
                        if (!vectorContains(timestamps, seg.timestamp))
                            timestamps.push_back(seg.timestamp);
                std::sort(timestamps.begin(), timestamps.end());

                for (int i = 0; i < 5; i++)
                {
                    auto &r = viirs_reader_imaging[i];
                    std::vector<viirs::VIIRS_Segment> oldSegments = r.segments;
                    r.segments.clear();

                    for (auto &time : timestamps)
                    {
                        bool contains = false;
                        for (auto &seg : oldSegments)
                        {
                            if (seg.timestamp == time)
                            {
                                r.segments.push_back(seg);
                                contains = true;
                            }
                        }

                        if (!contains)
                            r.segments.push_back(viirs::VIIRS_Segment(r.channelSettings));
                    }
                    r.timestamps = timestamps;
                }

                for (int i = 0; i < 16; i++)
                {
                    auto &r = viirs_reader_moderate[i];
                    std::vector<viirs::VIIRS_Segment> oldSegments = r.segments;
                    r.segments.clear();

                    for (auto &time : timestamps)
                    {
                        bool contains = false;
                        for (auto &seg : oldSegments)
                        {
                            if (seg.timestamp == time)
                            {
                                r.segments.push_back(seg);
                                contains = true;
                            }
                        }

                        if (!contains)
                            r.segments.push_back(viirs::VIIRS_Segment(r.channelSettings));
                    }
                    r.timestamps = timestamps;
                }

                for (int i = 0; i < 3; i++)
                {
                    auto &r = viirs_reader_dnb[i];
                    std::vector<viirs::VIIRS_Segment> oldSegments = r.segments;
                    r.segments.clear();

                    for (auto &time : timestamps)
                    {
                        bool contains = false;
                        for (auto &seg : oldSegments)
                        {
                            if (seg.timestamp == time)
                            {
                                r.segments.push_back(seg);
                                contains = true;
                            }
                        }

                        if (!contains)
                            r.segments.push_back(viirs::VIIRS_Segment(r.channelSettings));
                    }
                    r.timestamps = timestamps;
                }
            }

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

        std::string JPSSInstrumentsDecoderModule::getID() { return "jpss_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> JPSSInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<JPSSInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace jpss