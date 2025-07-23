#include "module_earthcare_instruments.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/tracking/tle.h"
#include "common/utils.h"
#include "core/resources.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include "utils/stats.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "image/io.h"

namespace earthcare
{
    namespace instruments
    {
        EarthCAREInstrumentsDecoderModule::EarthCAREInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void EarthCAREInstrumentsDecoderModule::process()
        {
            uint8_t cadu[1279];

            // Demuxers
            ccsds::ccsds_tm::Demuxer demuxer_vcid5(1109, false);
            ccsds::ccsds_tm::Demuxer demuxer_vcid6(1109, false);

            // std::ofstream output("file.ccsds");

            //  std::ofstream idk_test_out(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/test_idk.bin", std::ios::binary);

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)cadu, 1279);

                // Parse this transport frame
                ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // logger->info(vcdu.vcid);

                if (vcdu.vcid == 5) // MSI VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid5.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1100)
                            msi_reader.work(pkt);
                        // else
                        //     logger->error("%d  %d", pkt.header.apid, pkt.payload.size());
                    }
                }

                if (vcdu.vcid == 6) // ATLID VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid6.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1228)
                            atlid_reader.work(pkt);
                        // else
                        //     logger->error("%d  %d", pkt.header.apid, pkt.payload.size());
                    }
                }
            }

            cleanup();

            // Products dataset
            satdump::products::DataSet dataset;
            dataset.satellite_name = "EarthCARE";
            dataset.timestamp = satdump::get_median(msi_reader.timestamps);

            int norad = 59908;

            // Satellite ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : EarthCARE");
            }

            // MSI
            {
                msi_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSI";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MSI");
                logger->info("Lines : " + std::to_string(msi_reader.lines));

                satdump::products::ImageProduct msi_products;
                msi_products.instrument_name = "earthcare_msi";
                msi_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/earthcare_msi.json")),
                                                         satdump::general_tle_registry->get_from_norad_time(norad, dataset.timestamp), msi_reader.timestamps);

                for (int i = 0; i < 7; i++)
                    msi_products.images.push_back({i, "MSI-" + std::to_string(i + 1), std::to_string(i + 1), msi_reader.getChannel(i), 16});

                msi_products.save(directory);
                dataset.products_list.push_back("MSI");

                msi_status = DONE;
            }

            // ATLID
            {
                atlid_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ATLID";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- ATLID");
                logger->info("Lines : " + std::to_string(atlid_reader.lines));

                auto img = atlid_reader.getChannel();
                image::save_png(img, directory + "/ATLID.png");

                atlid_status = DONE;
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void EarthCAREInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("EarthCARE Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##earthcareinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Images / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MSI");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", msi_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(msi_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("ATLID");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", atlid_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(atlid_status);

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string EarthCAREInstrumentsDecoderModule::getID() { return "earthcare_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> EarthCAREInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<EarthCAREInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace earthcare