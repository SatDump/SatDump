#include "module_ominous_instruments.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/tracking/tle.h"
#include "common/utils.h"
#include "core/resources.h"
#include "imgui/imgui.h"
#include "init.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include "utils/stats.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "image/io.h"

namespace ominous
{
    namespace instruments
    {
        OminousInstrumentsDecoderModule::OminousInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void OminousInstrumentsDecoderModule::process()
        {
            uint8_t cadu[259];

            // Demuxers
            ccsds::ccsds_aos::Demuxer demuxer_vcid5(215, false);
            ccsds::ccsds_aos::Demuxer demuxer_vcid6(215, false);
            ccsds::ccsds_aos::Demuxer demuxer_vcid7(215, false);
            ccsds::ccsds_aos::Demuxer demuxer_vcid8(215, false);

            std::ofstream output("file.ccsds");

            //  std::ofstream idk_test_out(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/test_idk.bin", std::ios::binary);

            loris_reader.directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/LORIS";
            if (!std::filesystem::exists(loris_reader.directory))
                std::filesystem::create_directories(loris_reader.directory);

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)cadu, 259);

                // Parse this transport frame
                ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // logger->info(vcdu.vcid);

                if (vcdu.vcid == 5) // CHEAP VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid5.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1)
                            csr_reader.work(pkt);
                        else if (pkt.header.apid >= 101 && pkt.header.apid <= 110)
                            csr_lossy_reader.work(pkt);
                    }
                }
                else if (vcdu.vcid == 6) // LORIS VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid6.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 2)
                            loris_reader.work(pkt);
                    }
                }
                else if (vcdu.vcid == 7) // CHEAP VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid7.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1)
                        {
                            // pkt.payload.resize(65536);
                            // output.write((char *)pkt.header.raw, 6);
                            // output.write((char *)pkt.payload.data(), 65536);
                        }
                    }
                }
                else if (vcdu.vcid == 8) // CHEAP VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid8.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1)
                        {
                            pkt.payload.resize(65536);
                            // output.write((char *)pkt.header.raw, 6);
                            output.write((char *)pkt.payload.data(), 65536);
                        }
                    }
                }
            }

            cleanup();

            // Products dataset
            satdump::products::DataSet dataset;
            dataset.satellite_name = "Ominous-Simulator";
            dataset.timestamp = satdump::get_median(csr_reader.timestamps);

            int norad = 59908;

            // Satellite ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : Ominous-Simulator");
            }

            // CSR
            {
                csr_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CSR";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- CSR");
                logger->info("Lines : " + std::to_string(csr_reader.lines));

                satdump::products::ImageProduct csr_products;
                csr_products.instrument_name = "csr";
                csr_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/earthcare_msi.json")),
                                                         satdump::db_tle->get_from_norad_time(norad, dataset.timestamp), csr_reader.timestamps);

                for (int i = 0; i < 10; i++)
                    csr_products.images.push_back({i, "CSR-" + std::to_string(i + 1), std::to_string(i + 1), csr_reader.getChannel(i), 16});

                csr_products.save(directory);
                dataset.products_list.push_back("CSR");

                csr_status = DONE;
            }

            // CSR Lossy
            {
                csr_lossy_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CSR_Lossy";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- CSR_Lossy");
                logger->info("Lines : " + std::to_string(csr_lossy_reader.segments));

                satdump::products::ImageProduct csr_lossy_products;
                csr_lossy_products.instrument_name = "csr";
                csr_lossy_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/earthcare_msi.json")),
                                                               satdump::db_tle->get_from_norad_time(norad, dataset.timestamp), csr_lossy_reader.timestamps);

                for (int i = 0; i < 10; i++)
                    csr_lossy_products.images.push_back({i, "CSR-" + std::to_string(i + 1), std::to_string(i + 1), csr_lossy_reader.getChannel(i), 16});

                csr_lossy_products.save(directory);
                dataset.products_list.push_back("CSR_Lossy");

                csr_lossy_status = DONE;
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void OminousInstrumentsDecoderModule::drawUI(bool window)
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
                ImGui::Text("CSR");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", csr_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(csr_status);

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string OminousInstrumentsDecoderModule::getID() { return "ominous_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> OminousInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<OminousInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace ominous