#include "module_ldcm_instruments.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

extern "C"
{
#include "libs/aec/szlib.h"
}

namespace ldcm
{
    namespace instruments
    {
        LDCMInstrumentsDecoderModule::LDCMInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void LDCMInstrumentsDecoderModule::process()
        {
            uint8_t cadu[1034];

            // Demuxers
            ccsds::ccsds_aos::Demuxer demuxer_vcid5(1022, true, 0);
            ccsds::ccsds_aos::Demuxer demuxer_vcid12(1022, true, 0);

            // std::ofstream output("file.ccsds");

            //  std::ofstream idk_test_out(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/test_idk.bin", std::ios::binary);

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)cadu, 1034);

                // Parse this transport frame
                ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // logger->info(vcdu.vcid);

                if (vcdu.vcid == 5) // TIRS VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid5.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    { // 1792, 1793, 1794,
                        if (pkt.header.apid == 1792)
                            tirs_reader1.work(pkt);
                        else if (pkt.header.apid == 1793)
                            tirs_reader2.work(pkt);
                        else if (pkt.header.apid == 1794)
                            tirs_reader3.work(pkt);
                        // else
                        //     logger->error("%d  %d", pkt.header.apid, pkt.payload.size());
                    }
                }
#if 0
                else if (vcdu.vcid == 12) // OLI VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid12.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        logger->error("%d  %d", pkt.header.apid, pkt.payload.size());

                        if (pkt.header.apid == 299)
                        {
                            std::vector<uint8_t> output_buf(6000 * 4);

                            SZ_com_t opts;
                            opts.bits_per_pixel = 16;
                            opts.pixels_per_scanline = 6000;
                            opts.pixels_per_block = 16;
                            opts.options_mask = SZ_ALLOW_K13_OPTION_MASK | SZ_MSB_OPTION_MASK | SZ_NN_OPTION_MASK | SZ_RAW_OPTION_MASK;

                            size_t outsize = output_buf.size();
                            int off = 20 - 6;
                            SZ_BufftoBuffDecompress(output_buf.data(), &outsize, &pkt.payload[off], pkt.payload.size() - off, &opts);

                            pkt.payload = output_buf;

                            idk_test_out.write((char *)pkt.header.raw, 6);
                            pkt.payload.resize(10000 - 6);
                            idk_test_out.write((char *)pkt.payload.data(), pkt.payload.size());
                        }
                    }
                }
#endif
            }

            cleanup();

            // Products dataset
            satdump::products::DataSet dataset;
            dataset.satellite_name = "LandSat-8/9";
            dataset.timestamp = time(0); // avg_overflowless(avhrr_reader.timestamps);

            // TIRS
            {
                tirs_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/TIRS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- TIRS");
                logger->info("Lines 1 : " + std::to_string(tirs_reader1.lines));
                logger->info("Lines 2 : " + std::to_string(tirs_reader2.lines));
                logger->info("Lines 3 : " + std::to_string(tirs_reader3.lines));

                satdump::products::ImageProduct tirs_products;
                tirs_products.instrument_name = "tirs";
                // tirs_products.set_tle(satellite_tle);
                // tirs_products.set_wavenumber(0, 1311.99);
                // tirs_products.set_timestamps(mhs_reader.timestamps);
                // tirs_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/metop_abc_mhs.json")));

                for (int i = 0; i < 3; i++)
                    tirs_products.images.push_back({i, "TIRS-" + std::to_string(i + 1), std::to_string(i + 1), tirs_reader1.getChannel(i), 12});
                for (int i = 0; i < 3; i++)
                    tirs_products.images.push_back({i + 3, "TIRS-" + std::to_string(i + 4), std::to_string(i + 4), tirs_reader2.getChannel(i), 12});
                for (int i = 0; i < 3; i++)
                    tirs_products.images.push_back({i + 6, "TIRS-" + std::to_string(i + 7), std::to_string(i + 7), tirs_reader3.getChannel(i), 12});

                tirs_products.save(directory);
                dataset.products_list.push_back("TIRS");

                tirs_status = DONE;
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void LDCMInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MATS Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##tirsinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
                ImGui::Text("TIRS 1");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", tirs_reader1.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(tirs_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("TIRS 2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", tirs_reader2.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(tirs_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("TIRS 3");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", tirs_reader3.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(tirs_status);

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string LDCMInstrumentsDecoderModule::getID() { return "ldcm_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> LDCMInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<LDCMInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace ldcm