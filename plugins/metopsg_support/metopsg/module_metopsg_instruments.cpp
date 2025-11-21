#include "module_metopsg_instruments.h"
#include "common/calibration.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/repack.h"
#include "common/tracking/tle.h"
#include "common/utils.h"
#include "core/resources.h"
#include "image/bowtie.h"
#include "image/image.h"
#include "image/io.h"
#include "image/processing.h"
#include "imgui/imgui.h"
#include "init.h"
#include "logger.h"
#include "metopsg.h"
#include "nlohmann/json_utils.h"
#include "products/image/channel_transform.h"
#include "utils/stats.h"
#include <filesystem>
#include <fstream>
#include <string>

#include "products/dataset.h"
#include "products/image_product.h"
#include "products/punctiform_product.h"

namespace metopsg
{
    namespace instruments
    {
        MetOpSGInstrumentsDecoderModule::MetOpSGInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            ignore_integrated_tle = parameters.contains("ignore_integrated_tle") ? parameters["ignore_integrated_tle"].get<bool>() : false;
            metimage_bowtie = parameters.contains("metimage_bowtie") ? parameters["metimage_bowtie"].get<bool>() : false;
            fsfsm_enable_output = false;
        }

        void MetOpSGInstrumentsDecoderModule::process()
        {
            uint8_t cadu[1024];

            // 3-MI TODOREWORK
            {
                std::string directory1 = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/3MI/1";
                std::string directory2 = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/3MI/2";
                if (!std::filesystem::exists(directory1))
                    std::filesystem::create_directories(directory1);
                if (!std::filesystem::exists(directory2))
                    std::filesystem::create_directories(directory2);
                threemi_reader.directory_1 = directory1;
                threemi_reader.directory_2 = directory2;
            }

            // Demuxers
            ccsds::ccsds_aos::Demuxer demuxer_vcid9(884, false);
            ccsds::ccsds_aos::Demuxer demuxer_vcid10(884, false);
            ccsds::ccsds_aos::Demuxer demuxer_vcid13(884, false);
            ccsds::ccsds_aos::Demuxer demuxer_vcid14(884, false);
            ccsds::ccsds_aos::Demuxer demuxer_vcid15(884, false);
            ccsds::ccsds_aos::Demuxer demuxer_vcid16(884, false);

            // Setup Admin Message
            {
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Admin Messages";
                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);
                admin_msg_reader.directory = directory;
            }

            std::vector<uint8_t> metop_scids;

            while (should_run())
            {
                // Read buffer
                read_data(cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

                if (vcdu.spacecraft_id == METOPSG_A1_SCID || vcdu.spacecraft_id == METOPSG_A2_SCID || vcdu.spacecraft_id == METOPSG_A3_SCID || //
                    vcdu.spacecraft_id == METOPSG_B1_SCID || vcdu.spacecraft_id == METOPSG_B2_SCID || vcdu.spacecraft_id == METOPSG_B3_SCID)
                    metop_scids.push_back(vcdu.spacecraft_id);

                if (vcdu.vcid == 9) // NAVATT
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid9.work(cadu);
                    // for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    //     logger->critical("APID %d LEN %d", pkt.header.apid, pkt.payload.size() + 6);
                }
                else if (vcdu.vcid == 13) // MWS
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid13.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        mws_reader.work(pkt);
                }
                else if (vcdu.vcid == 14) // 3-MI
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid14.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        threemi_reader.work(pkt);
                }
                else if (vcdu.vcid == 15) // Sentinel-5
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid15.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        sentinel5_reader.work(pkt);
                }
                else if (vcdu.vcid == 16) // METimage
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid16.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        metimage_reader.work(pkt);
                }
                else if (vcdu.vcid == 10) // Admin Messages
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid10.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 577)
                            admin_msg_reader.work(pkt);
                }
            }

            cleanup();

            int scid = satdump::most_common(metop_scids.begin(), metop_scids.end(), 0);
            metop_scids.clear();

            std::string sat_name = "Unknown MetOp-SG";
            if (scid == METOPSG_A1_SCID)
                sat_name = "MetOp-SG-A1";
            else if (scid == METOPSG_A2_SCID)
                sat_name = "MetOp-SG-A2";
            else if (scid == METOPSG_A3_SCID)
                sat_name = "MetOp-SG-A3";
            else if (scid == METOPSG_B1_SCID)
                sat_name = "MetOp-SG-B1";
            else if (scid == METOPSG_B2_SCID)
                sat_name = "MetOp-SG-B2";
            else if (scid == METOPSG_B3_SCID)
                sat_name = "MetOp-SG-B3";

            int norad = 0;
            if (scid == METOPSG_A1_SCID)
                norad = METOPSG_A1_NORAD;
            else if (scid == METOPSG_A2_SCID)
                norad = METOPSG_A2_NORAD;
            else if (scid == METOPSG_A3_SCID)
                norad = METOPSG_A3_NORAD;
            else if (scid == METOPSG_B1_SCID)
                norad = METOPSG_B1_NORAD;
            else if (scid == METOPSG_B2_SCID)
                norad = METOPSG_B2_NORAD;
            else if (scid == METOPSG_B3_SCID)
                norad = METOPSG_B3_NORAD;

            // Products dataset
            satdump::products::DataSet dataset;
            dataset.satellite_name = sat_name;
            dataset.timestamp = satdump::get_median(metimage_reader.timestamps);

            std::optional<satdump::TLE> satellite_tle; // TODOREWORK = admin_msg_reader.tles.get_from_norad(norad);
            if (!satellite_tle.has_value() || ignore_integrated_tle)
                satellite_tle = satdump::db_tle->get_from_norad_time(norad, dataset.timestamp);

            // Satellite ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : " + sat_name);
            }

            // Sentinel-5
            {
                sentinel5_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Sentinel-5";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- Sentinel-5");

                for (int i = 0; i < sentinel5_reader.nchannels; i++)
                {
                    std::string name = "num_" + std::to_string(i + 1);
                    auto img = sentinel5_reader.getChannel(i, name);
                    image::save_png(img, directory + "/" + name + ".png");
                }

                dataset.products_list.push_back("Sentinel-5");

                sentinel5_status = DONE;
            }

            // MWS
            {
                mws_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                mws_reader.correlate();

                logger->info("----------- MWS");
                logger->info("Lines : " + std::to_string(mws_reader.lines[0])); // TODOREWORK

                satdump::products::ImageProduct mws_products;
                mws_products.instrument_name = "metopsg_mws";
                mws_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/metopsg_a_mws.json")), satellite_tle, mws_reader.timestamps[0]);

                for (int i = 0; i < 24; i++)
                {
                    mws_products.images.push_back({i, "MWS-" + std::to_string(i + 1), std::to_string(i + 1), mws_reader.getChannel(i), 16, //
                                                   i < 2 ? satdump::ChannelTransform().init_none() : satdump::ChannelTransform().init_affine(16, 1, 0, 0)});
                    // mws_products.set_channel_unit(i, i < 3 ? CALIBRATION_ID_REFLECTIVE_RADIANCE : CALIBRATION_ID_EMISSIVE_RADIANCE);
                    //  mws_products.set_channel_wavenumber(i, calib_coefs[sat_name]["channels"][i]["wavnb"]);
                }

                mws_products.save(directory);
                dataset.products_list.push_back("MWS");

                mws_status = DONE;
            }

            // METimage
            {
                metimage_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/METimage";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- METimage");
                logger->info("Segments : " + std::to_string(metimage_reader.segments)); // TODOREWORK

                satdump::products::ImageProduct metimage_products;
                metimage_products.instrument_name = "metimage";
                metimage_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/metopsg_a_metimage.json")), satellite_tle, metimage_reader.timestamps);

                const float ch_offsets[20] = {
                    0,   //
                    -6,  //
                    -15, //
                    -23, //
                    -34, //
                    -26, //
                    -35, //
                    -26, //
                    -23, //
                    -24, //
                    -21, //
                    -16, //
                    -12, //
                    -8,  //
                    0,   // TODO
                    -18, //
                    -24, //
                    0,   // TODO
                    -28, //
                    0,   // TODO
                };

                // BowTie values
                const float alpha = 1.0 / 1.8;
                const float beta = 0.58333;
                const long scanHeight_500 = 24;

                for (int i = 0; i < 20; i++)
                {
                    int vii = metimage_reader.out_ch_n[i];
                    auto img = metimage_reader.getChannel(i);
                    if (metimage_bowtie)
                        img = image::bowtie::correctGenericBowTie(img, 1, scanHeight_500, alpha, beta);
                    metimage_products.images.push_back({i, "METimage-" + std::to_string(vii), std::to_string(vii), img, 16, //
                                                        satdump::ChannelTransform().init_affine(1, 1, ch_offsets[i], 0)});
                    // mws_products.set_channel_unit(i, i < 3 ? CALIBRATION_ID_REFLECTIVE_RADIANCE : CALIBRATION_ID_EMISSIVE_RADIANCE);
                    //  mws_products.set_channel_wavenumber(i, calib_coefs[sat_name]["channels"][i]["wavnb"]);
                }

                metimage_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/metopsg_a_metimage.json")), satellite_tle, metimage_reader.timestamps);

                metimage_products.save(directory);
                dataset.products_list.push_back("METimage");

                metimage_status = DONE;
            }

            // Admin Messages
            {
                admin_msg_status = DONE;
                logger->info("----------- Admin Message");
                logger->info("Count : " + std::to_string(admin_msg_reader.count));
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void MetOpSGInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp-SG Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##metopsginstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
                ImGui::Text("METimage");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", metimage_reader.segments);
                ImGui::TableSetColumnIndex(2);
                drawStatus(metimage_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MWS");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", mws_reader.lines[0]);
                ImGui::TableSetColumnIndex(2);
                drawStatus(mws_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("3MI");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", threemi_reader.img_n);
                ImGui::TableSetColumnIndex(2);
                drawStatus(threemi_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Sentinel-5");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", sentinel5_reader.total_packets);
                ImGui::TableSetColumnIndex(2);
                drawStatus(sentinel5_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Admin Messages");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", admin_msg_reader.count);
                ImGui::TableSetColumnIndex(2);
                drawStatus(admin_msg_status);

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string MetOpSGInstrumentsDecoderModule::getID() { return "metopsg_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> MetOpSGInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MetOpSGInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace metopsg