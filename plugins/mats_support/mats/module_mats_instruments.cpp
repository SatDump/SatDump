#include "module_mats_instruments.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include <cstdint>
#include <filesystem>

namespace mats
{
    namespace instruments
    {
        MATSInstrumentsDecoderModule::MATSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void MATSInstrumentsDecoderModule::process()
        {
            uint8_t cadu[1279];

            // Demuxers
            ccsds::ccsds_tm::Demuxer demuxer_vcid1(1103, false);

            // std::ofstream output("file.ccsds");

            std::string mats_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1279);

                // Parse this transport frame
                ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // logger->info(vcdu.vcid);

                if (vcdu.vcid == 1) // Replay VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 100)
                            mats_reader.work(pkt, mats_directory);
                    }
                }
            }

            cleanup();

            // Products dataset
            satdump::products::DataSet dataset;
            dataset.satellite_name = "MATS";
            dataset.timestamp = time(0); // avg_overflowless(avhrr_reader.timestamps);

            // NADIR Imager
            {
                mats_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Nadir";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MATS Nadir");
                logger->info("Lines : " + std::to_string(mats_reader.nadir_lines));

                satdump::products::ImageProduct mats_nadir_products;
                mats_nadir_products.instrument_name = "mats_nadir";
                // mats_nadir_products.set_tle(satellite_tle);
                // mats_nadir_products.set_timestamps(mhs_reader.timestamps);
                // mats_nadir_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/metop_abc_mhs.json")));

                mats_nadir_products.images.push_back({0, "MATS-Nadir", "1", mats_reader.getNadirImage(), 12});
                mats_nadir_products.set_channel_wavenumber(0, 1311.99);

                mats_nadir_products.save(directory);
                dataset.products_list.push_back("Nadir");

                mats_status = DONE;
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void MATSInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MATS Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##matsinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Images / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                for (int i = 0; i < 7; i++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MATS %s", mats::channel_names[i].c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mats_reader.img_cnts[i]);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mats_status);
                }

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string MATSInstrumentsDecoderModule::getID() { return "mats_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> MATSInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MATSInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace mats