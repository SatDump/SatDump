#include "module_hinode_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_weather/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/ccsds/ccsds_weather/demuxer.h"
#include "products/products.h"
#include "products/dataset.h"

#include "common/codings/reedsolomon/reedsolomon.h"

namespace hinode
{
    namespace instruments
    {
        HinodeInstrumentsDecoderModule::HinodeInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        inline void handleAPID(ccsds::CCSDSPacket &pkt, HinodeDepacketizer &depack, std::string &directory, ImageRecomposer &recomp)
        {
            hinode::DecodedImage result;
            /*int status =*/depack.work(pkt, &result);
            // printf("status %d - %d\n", status, result.apid);
            if (result.apid != -1)
            {
                result.img.save_img(directory + "/" + std::to_string(depack.img_cnt));
                saveJsonFile(directory + "/" + std::to_string(depack.img_cnt) + ".json", nlohmann::json(result.sci));
                image::Image<uint16_t> img;
                if (recomp.pushSegment(result, &img))
                    img.save_img(directory + "/full_" + std::to_string(result.sci.MainID));
            }
        }

        void HinodeInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1024];

            // Demuxers
            ccsds::ccsds_weather::Demuxer demuxer_vcid4(880, true, 0);

            reedsolomon::ReedSolomon rs_check(reedsolomon::RS223);

            std::string flt_obs1_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/FLT/OBS_1";
            if (!std::filesystem::exists(flt_obs1_directory))
                std::filesystem::create_directories(flt_obs1_directory);
            std::string flt_obs2_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/FLT/OBS_2";
            if (!std::filesystem::exists(flt_obs2_directory))
                std::filesystem::create_directories(flt_obs2_directory);

            std::string spp_obs1_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SPP/OBS_1";
            if (!std::filesystem::exists(spp_obs1_directory))
                std::filesystem::create_directories(spp_obs1_directory);
            std::string spp_obs2_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SPP/OBS_2";
            if (!std::filesystem::exists(spp_obs2_directory))
                std::filesystem::create_directories(spp_obs2_directory);

            std::string xrt_obs1_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/XRT/OBS_1";
            if (!std::filesystem::exists(xrt_obs1_directory))
                std::filesystem::create_directories(xrt_obs1_directory);
            std::string xrt_obs2_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/XRT/OBS_2";
            if (!std::filesystem::exists(xrt_obs2_directory))
                std::filesystem::create_directories(xrt_obs2_directory);

            std::string eis_obs1_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/EIS/OBS_1";
            if (!std::filesystem::exists(eis_obs1_directory))
                std::filesystem::create_directories(eis_obs1_directory);
            std::string eis_obs2_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/EIS/OBS_2";
            if (!std::filesystem::exists(eis_obs2_directory))
                std::filesystem::create_directories(eis_obs2_directory);

            ImageRecomposer recomp_flt_obs1, recomp_flt_obs2;
            ImageRecomposer recomp_spp_obs1, recomp_spp_obs2;
            ImageRecomposer recomp_xrt_obs1, recomp_xrt_obs2;
            ImageRecomposer recomp_eis_obs1, recomp_eis_obs2;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)cadu, 1024);

                // Check RS
                int errors[4];
                rs_check.decode_interlaved(cadu, true, 4, errors);
                if (errors[0] < 0 || errors[1] < 0 || errors[2] < 0 || errors[3] < 0)
                    continue;

                // Parse this transport frame
                ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // printf("VCID %d\n", vcdu.vcid);

                if (vcdu.vcid == 4)
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid4.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if ((int)pkt.payload.size() != pkt.header.packet_length + 1)
                            continue; // Strict filtering

                        // printf("APID %d - %d\n", pkt.header.apid, pkt.payload.size());

                        if (pkt.header.apid == 332) // FLT OBS1
                            handleAPID(pkt, depack_flt_obs1, flt_obs1_directory, recomp_flt_obs1);
                        else if (pkt.header.apid == 333) // FLT OBS2
                            handleAPID(pkt, depack_flt_obs2, flt_obs2_directory, recomp_flt_obs2);

                        if (pkt.header.apid == 334) // SPP OBS1
                            handleAPID(pkt, depack_spp_obs1, spp_obs1_directory, recomp_spp_obs1);
                        else if (pkt.header.apid == 335) // SPP OBS2
                            handleAPID(pkt, depack_spp_obs2, spp_obs2_directory, recomp_spp_obs2);

                        if (pkt.header.apid == 426) // XRT OBS1
                            handleAPID(pkt, depack_xrt_obs1, xrt_obs1_directory, recomp_xrt_obs1);
                        else if (pkt.header.apid == 427) // XRT OBS2
                            handleAPID(pkt, depack_xrt_obs2, xrt_obs2_directory, recomp_xrt_obs2);

                        if (pkt.header.apid == 458) // EIS OBS1
                            handleAPID(pkt, depack_eis_obs1, eis_obs1_directory, recomp_eis_obs1);
                        else if (pkt.header.apid == 459) // EIS OBS2
                            handleAPID(pkt, depack_eis_obs2, eis_obs2_directory, recomp_eis_obs2);
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
        }

        void HinodeInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Hinode Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##hinodeinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
                ImGui::Text("FLT OBS1");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", depack_flt_obs1.img_cnt);
                ImGui::TableSetColumnIndex(2);
                drawStatus(general_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("FLT OBS2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", depack_flt_obs2.img_cnt);
                ImGui::TableSetColumnIndex(2);
                drawStatus(general_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("SPP OBS1");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", depack_spp_obs1.img_cnt);
                ImGui::TableSetColumnIndex(2);
                drawStatus(general_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("SPP OBS2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", depack_spp_obs2.img_cnt);
                ImGui::TableSetColumnIndex(2);
                drawStatus(general_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("XRT OBS1");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", depack_xrt_obs1.img_cnt);
                ImGui::TableSetColumnIndex(2);
                drawStatus(general_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("XRT OBS2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", depack_xrt_obs2.img_cnt);
                ImGui::TableSetColumnIndex(2);
                drawStatus(general_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("EIS OBS1");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", depack_eis_obs1.img_cnt);
                ImGui::TableSetColumnIndex(2);
                drawStatus(general_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("EIS OBS2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", depack_eis_obs2.img_cnt);
                ImGui::TableSetColumnIndex(2);
                drawStatus(general_status);

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string HinodeInstrumentsDecoderModule::getID()
        {
            return "hinode_instruments";
        }

        std::vector<std::string> HinodeInstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> HinodeInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<HinodeInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop