#include "module_jason3_instruments.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/utils.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "utils/stats.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include <thread>

#include "common/tracking/tle.h"
#include "core/resources.h"
#include "nlohmann/json_utils.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include "products/punctiform_product.h"

namespace jason3
{
    namespace instruments
    {
        Jason3InstrumentsDecoderModule::Jason3InstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void Jason3InstrumentsDecoderModule::process()
        {
            uint8_t cadu[1279];

            // Demuxers
            ccsds::ccsds_tm::Demuxer demuxer_vcid1(1101, false, 2, 4);
            ccsds::ccsds_tm::Demuxer demuxer_vcid2(1101, false, 2, 4);

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1279);

                // Parse this transport frame
                ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

                if (vcdu.vcid == 1) // AMR-2, Poseidon
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1408) // AMR-2
                            amr2_reader.work(pkt);
                        else if (pkt.header.apid == 1031) // Poseidon-C
                            poseidon_c_reader.work(pkt);
                        else if (pkt.header.apid == 1032) // Poseidon-Ku
                            poseidon_ku_reader.work(pkt);
                    }
                }
                else if (vcdu.vcid == 2) // LPT
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid2.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1316)
                            lpt_els_a_reader.work(pkt);
                        else if (pkt.header.apid == 1317)
                            lpt_els_b_reader.work(pkt);
                        else if (pkt.header.apid == 1318)
                            lpt_aps_a_reader.work(pkt);
                        else if (pkt.header.apid == 1319)
                            lpt_aps_b_reader.work(pkt);
                    }
                }
            }

            cleanup();

            // Products dataset
            satdump::products::DataSet dataset;
            dataset.satellite_name = "Jason-3";
            dataset.timestamp = satdump::get_median(amr2_reader.timestamps);

            std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry->get_from_norad_time(41240, dataset.timestamp);

            /* // AMR-2
             {
                 amr2_status = SAVING;
                 std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AMR-2";

                 if (!std::filesystem::exists(directory))
                     std::filesystem::create_directory(directory);

                 logger->info("----------- AMR-2");
                 logger->info("Lines : " + std::to_string(amr2_reader.lines));

                 satdump::products::ImageProduct amr2_products;
                 amr2_products.instrument_name = "amr_2";
                 amr2_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/jason3_amr2.json")), satellite_tle, amr2_reader.timestamps);

                 for (int i = 0; i < 3; i++)
                     amr2_products.images.push_back({i, "AMR2-" + std::to_string(i + 1), std::to_string(i + 1), amr2_reader.getChannel(i), 16});

                 amr2_products.save(directory);
                 dataset.products_list.push_back("AMR-2");

                 amr2_status = DONE;
             }*/

            // AMR-2
            {
                amr2_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AMR-2";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- AMR-2");
                logger->info("Lines : " + std::to_string(amr2_reader.lines));

                satdump::products::PunctiformProduct amr2_products;
                amr2_products.instrument_name = "amr2";
                amr2_products.set_tle(satellite_tle);

                /*for (int i = 0; i < 3; i++)
                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    // TODOREWORK                      h.abs_index = i;
                    h.channel_name = std::to_string(i + 1);
                    h.timestamps = amr2_reader.timestamps;
                    auto img = amr2_reader.getChannel(i);
                    for (int i2 = 0; i2 < amr2_reader.timestamps.size(); i2++)
                        h.data.push_back((double)img.get(0, 0, i2));
                    amr2_products.data.push_back(h);
                }*/

                for (int i = 0; i < 3; i++)
                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = std::to_string(i + 1);
                    h.timestamps = amr2_reader.timestamps_data;
                    h.data = amr2_reader.channels_data[i];
                    amr2_products.data.push_back(h);
                }

                amr2_products.save(directory);
                dataset.products_list.push_back("AMR-2");

                amr2_status = DONE;
            }

            // LPT
            {
                lpt_aps_b_status = lpt_aps_a_status = lpt_els_b_status = lpt_els_a_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/LPT";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- LPT");
                logger->info("ELS-A : " + std::to_string(lpt_els_a_reader.timestamps.size()));
                logger->info("ELS-B : " + std::to_string(lpt_els_b_reader.timestamps.size()));
                logger->info("APS-A : " + std::to_string(lpt_aps_a_reader.timestamps.size()));
                logger->info("APS-B : " + std::to_string(lpt_aps_b_reader.timestamps.size()));

                satdump::products::PunctiformProduct lpt_products;
                lpt_products.instrument_name = "jason3_lpt";
                lpt_products.set_tle(satellite_tle);

                for (int i = 0; i < lpt_els_a_reader.channel_counts.size(); i++)
                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = "ELS-A " + std::to_string(i + 1);
                    h.timestamps = lpt_els_a_reader.timestamps;
                    for (int x = 0; x < lpt_els_a_reader.channel_counts[i].size(); x++)
                        h.data.push_back(lpt_els_a_reader.channel_counts[i][x]);
                    lpt_products.data.push_back(h);
                }

                for (int i = 0; i < lpt_els_b_reader.channel_counts.size(); i++)
                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = "ELS-B " + std::to_string(i + 1);
                    h.timestamps = lpt_els_b_reader.timestamps;
                    for (int x = 0; x < lpt_els_b_reader.channel_counts[i].size(); x++)
                        h.data.push_back(lpt_els_b_reader.channel_counts[i][x]);
                    lpt_products.data.push_back(h);
                }

                for (int i = 0; i < lpt_aps_a_reader.channel_counts.size(); i++)
                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = "APS-A " + std::to_string(i + 1);
                    h.timestamps = lpt_aps_a_reader.timestamps;
                    for (int x = 0; x < lpt_aps_a_reader.channel_counts[i].size(); x++)
                        h.data.push_back(lpt_aps_a_reader.channel_counts[i][x]);
                    lpt_products.data.push_back(h);
                }

                for (int i = 0; i < lpt_aps_b_reader.channel_counts.size(); i++)
                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = "APS-B " + std::to_string(i + 1);
                    h.timestamps = lpt_aps_b_reader.timestamps;
                    for (int x = 0; x < lpt_aps_b_reader.channel_counts[i].size(); x++)
                        h.data.push_back(lpt_aps_b_reader.channel_counts[i][x]);
                    lpt_products.data.push_back(h);
                }

                lpt_products.save(directory);
                dataset.products_list.push_back("LPT");

                lpt_aps_b_status = lpt_aps_a_status = lpt_els_b_status = lpt_els_a_status = DONE;
            }

            // Poseidon-3
            {
                poseidon_c_status = poseidon_ku_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Poseidon-3";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- Poseidon-3");
                logger->info("C-Band  : " + std::to_string(poseidon_c_reader.timestamps.size()));
                logger->info("Ku-Band : " + std::to_string(poseidon_ku_reader.timestamps.size()));

                satdump::products::PunctiformProduct poseidon_products;
                poseidon_products.instrument_name = "poseidon3_todorework";
                poseidon_products.set_tle(satellite_tle);

                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = "C-Band Scatter";
                    h.timestamps = poseidon_c_reader.timestamps;
                    h.data = poseidon_c_reader.data_scatter;
                    poseidon_products.data.push_back(h);
                }

                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = "C-Band Height";
                    h.timestamps = poseidon_c_reader.timestamps;
                    h.data = poseidon_c_reader.data_height;
                    poseidon_products.data.push_back(h);
                }

                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = "C-Band Unknon";
                    h.timestamps = poseidon_c_reader.timestamps;
                    h.data = poseidon_c_reader.data_unknown;
                    poseidon_products.data.push_back(h);
                }

                /////////////////////////////

                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = "Ku-Band Scatter";
                    h.timestamps = poseidon_ku_reader.timestamps;
                    h.data = poseidon_ku_reader.data_scatter;
                    poseidon_products.data.push_back(h);
                }

                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = "Ku-Band Height";
                    h.timestamps = poseidon_ku_reader.timestamps;
                    h.data = poseidon_ku_reader.data_height;
                    poseidon_products.data.push_back(h);
                }

                {
                    satdump::products::PunctiformProduct::DataHolder h;
                    h.channel_name = "Ku-Band Unknon";
                    h.timestamps = poseidon_ku_reader.timestamps;
                    h.data = poseidon_ku_reader.data_unknown;
                    poseidon_products.data.push_back(h);
                }

                poseidon_products.save(directory);
                dataset.products_list.push_back("Poseidon-3");

                poseidon_c_status = poseidon_ku_status = DONE;
            }

            // TODOREWORK POSEIDON
            logger->critical("TODOREWORK POSEIDON as limb!!!");
            // std::this_thread::sleep_for(std::chrono::seconds(10));

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void Jason3InstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Jason-3 Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##jason3instrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
                ImGui::Text("AMR-2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", amr2_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(amr2_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Poseidon C");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", poseidon_c_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(poseidon_c_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Poseidon Ku");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", poseidon_ku_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(poseidon_ku_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("LPT ELS-A");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", lpt_els_a_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(lpt_els_a_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("LPT ELS-B");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", lpt_els_b_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(lpt_els_b_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("LPT APS-A");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", lpt_aps_a_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(lpt_aps_a_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("LPT APS-B");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", lpt_aps_b_reader.frames);
                ImGui::TableSetColumnIndex(2);
                drawStatus(lpt_aps_b_status);

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string Jason3InstrumentsDecoderModule::getID() { return "jason3_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> Jason3InstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<Jason3InstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace jason3