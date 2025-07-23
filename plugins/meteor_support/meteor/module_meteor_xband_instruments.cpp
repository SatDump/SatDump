#include "module_meteor_xband_instruments.h"
#include "common/repack.h"
#include "common/tracking/tle.h"
#include "common/utils.h"
#include "core/resources.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "meteor.h"
#include "nlohmann/json_utils.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include "utils/stats.h"
#include "utils/time.h"
#include <filesystem>
#include <fstream>

namespace meteor
{
    namespace instruments
    {
        MeteorXBandInstrumentsDecoderModule::MeteorXBandInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
            d_instrument_mode = parseDumpType(parameters);
        }

        void MeteorXBandInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            std::string sat_name = "Unknown Meteor";
            if (d_parameters["satellite_number"].get<std::string>() == "M2-2")
                sat_name = "METEOR-M2-2";
            else if (d_parameters["satellite_number"].get<std::string>() == "M2-3")
                sat_name = "METEOR-M2-3";
            else if (d_parameters["satellite_number"].get<std::string>() == "M2-4")
                sat_name = "METEOR-M2-4";

            int norad = 0;
            if (d_parameters["satellite_number"].get<std::string>() == "M2-2")
                norad = 44387; // M2-2
            else if (d_parameters["satellite_number"].get<std::string>() == "M2-3")
                norad = 57166; // M2-3
            else if (d_parameters["satellite_number"].get<std::string>() == "M2-4")
                norad = 59051; // M2-4

            if (d_instrument_mode == DUMP_TYPE_MTVZA)
            {

                uint8_t frame[380];

                std::vector<uint16_t> mtvza_channels[46];

                for (int i = 0; i < 46; i++)
                    mtvza_channels[i].resize(200);

                double last_timestamp = 0;
                std::vector<double> timestamps;

                time_t lastTime = 0;
                while (!data_in.eof())
                {
                    // Read buffer
                    data_in.read((char *)&frame, 380);

                    {
                        int counter = frame[4];

                        int pos = 10;
                        for (int c = 0; c < 46; c++)
                        {
                            for (int p = 0; p < 4; p++)
                            {
                                uint16_t val = (frame[pos + p * 92 + c * 2 + 0] << 8 | frame[pos + p * 92 + c * 2 + 1]); //+ 32768;
                                // int16_t val2 = *((int16_t *)&val);
                                if (counter < 51 && counter > 0)
                                    mtvza_channels[c][mtvza_lines * 200 + (counter - 1) * 4 + p] = val; // (int)val2; // + 32768;
                            }
                        }

                        if (counter == 51)
                        {
                            mtvza_lines++;

                            for (int i = 0; i < 46; i++)
                                mtvza_channels[i].resize((mtvza_lines + 1) * 200);
                            timestamps.push_back(last_timestamp);

                            logger->info("Lines %d CNT %d", mtvza_lines, counter);
                        }
                        else if (counter == 0)
                        {
                            uint16_t vidk1 = frame[12] << 8 | frame[13];
                            //  uint16_t vidk2 = frame[36] << 8 | frame[37];
                            uint32_t vidk2 = frame[14] << 8 | frame[15] << 0; //| frame[16] << 8 | frame[17];
                            // uint16_t counter = frame[6] << 8 | frame[7];

                            double vtidk = -1;

                            if (norad == 57166)
                                vtidk = 16708 * 24 * 3600 + (vidk2) * 65536 + vidk1 + 189 * 60 - 101.1 * 10 * 60;
                            else if (norad == 59051)
                                vtidk = 16615 * 24 * 3600 + (vidk2) * 65536 + vidk1 - 24 * 60; //+ 189 * 60 - 101.1 * 10 * 60;

                            last_timestamp = vtidk;

                            // logger->trace("%f ---- %f               %f", last_vidk, vtidk - last_vidk, last_timestamp);

                            // printf("--------------------------------------------------------- TIMET %s \n", timestamp_to_string(last_timestamp).c_str());
                            //  last_vidk = vtidk;
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

#if 0
            // Identify satellite, and apply per-sat settings...
            int msumr_serial_number;
            if (msumr_ids.empty())
                msumr_serial_number = -1;
            else
            {
                msumr_serial_number = most_common(msumr_ids.begin(), msumr_ids.end());
                msumr_ids.clear();
            }
#endif

                // Products dataset
                satdump::products::DataSet dataset;
                dataset.satellite_name = sat_name;
                dataset.timestamp = time(0); // avg_overflowless(msumr_timestamps);

                // MTVZA
                {
                    mtvza_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MTVZA";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directory(directory);

                    logger->info("----------- MTVZA");
                    logger->info("Lines : " + std::to_string(mtvza_lines));

                    satdump::products::ImageProduct mtvza_products;
                    mtvza_products.instrument_name = "mtvza";
                    mtvza_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-3_mtvza_dump.json")),
                                                               satdump::general_tle_registry->get_from_norad(norad), timestamps);

                    for (int i = 0; i < 46; i++)
                        mtvza_products.images.push_back({i, "MTVZA-" + std::to_string(i + 1), std::to_string(i + 1), image::Image(mtvza_channels[i].data(), 16, 200, mtvza_lines, 1), 16});

                    mtvza_products.save(directory);
                    dataset.products_list.push_back("MTVZA");

                    mtvza_status = DONE;
                }

                dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
            }
            else if (d_instrument_mode == DUMP_TYPE_KMSS_BPSK)
            {
                std::vector<uint16_t> msu100_1_dat[3];
                std::vector<uint16_t> msu100_2_dat[3];

                uint8_t frame[481280 / 8];

                uint16_t line_buffer_raw1[4000 * 3];
                uint16_t line_buffer_raw2[4000 * 3];

                int kmss_lines = 0;

                long last_subsecond_cnt = -1;

                std::ofstream header_out(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/hdr.raw");

                std::vector<double> timestamps;

                time_t lastTime = 0;
                while (!data_in.eof())
                {
                    // Read buffer
                    data_in.read((char *)&frame, 15040 * 4);

                    for (int f = 0; f < 2; f++)
                    {
                        uint8_t *cfrm1 = &frame[(f * 2 + 0) * 15040];
                        uint8_t *cfrm2 = &frame[(f * 2 + 1) * 15040];

                        repackBytesTo10bits(&cfrm1[/*75*/ 40], 15000, line_buffer_raw1);
                        repackBytesTo10bits(&cfrm2[/*75*/ 40], 15000, line_buffer_raw2);

                        if (f == 0)
                        {

                            uint32_t seconds_value = /* cfrm2[1] << 24 |*/ cfrm2[4] << 8 | cfrm2[5]; // << 8 | cfrm2[5];
                            uint32_t subseconds_value = cfrm2[33] << 16 | cfrm2[34] << 8 | cfrm2[35];

                            uint8_t *cfrm3 = &frame[2 * 15040];
                            uint32_t idk_value = cfrm3[4] << 8 | cfrm3[5];

                            if (last_subsecond_cnt != -1)
                            {
                                double offset = 0;
                                if (norad == 44387)
                                    offset = 1704067200 - 3600 * 24 - 3600 * 3;
                                else if (norad == 59051)
                                    offset = 1704067200 - 3600 * 24 - 3600 * 3 - 3600 * 24 * 3107;
                                double timestamp = offset + double(idk_value) * 65536 + double(seconds_value) + double(last_subsecond_cnt) / 16777216.0;
                                logger->trace("%s - %d", satdump::timestamp_to_string(timestamp).c_str(), idk_value);
                                timestamps.push_back(timestamp);
                            }
                            else
                            {
                                timestamps.push_back(-1);
                            }
                            last_subsecond_cnt = subseconds_value;

                            // for (int i = 0; i < 4; i++)
                            {
                                int i = 2;
                                uint8_t *f1 = &frame[i * 15040];

                                header_out.write((char *)f1, 39);
                            }
                        }

                        for (int x = 0; x < 4000; x++)
                        {
                            if (f == 0)
                            {
                                /*    msu100_1_dat[0].push_back(line_buffer_raw1[x * 3 + 0] << 6);
                                    msu100_1_dat[0].push_back(line_buffer_raw1[x * 3 + 0] << 6);

                                    msu100_1_dat[1].push_back(line_buffer_raw1[x * 3 + 1] << 6);
                                    msu100_1_dat[1].push_back(line_buffer_raw1[x * 3 + 1] << 6);

                                    msu100_1_dat[2].push_back(line_buffer_raw1[x * 3 + 2] << 6);
                                    msu100_1_dat[2].push_back(line_buffer_raw1[x * 3 + 2] << 6);*/

                                msu100_1_dat[0].push_back(line_buffer_raw1[x * 3 + 0] << 6);
                                msu100_1_dat[1].push_back(line_buffer_raw1[x * 3 + 1] << 6);
                                msu100_1_dat[2].push_back(line_buffer_raw2[x * 3 + 0] << 6);

                                msu100_1_dat[0].push_back(line_buffer_raw2[x * 3 + 1] << 6);
                                msu100_1_dat[1].push_back(line_buffer_raw2[x * 3 + 2] << 6);
                                msu100_1_dat[2].push_back(line_buffer_raw1[x * 3 + 2] << 6);
                            }
                            else if (f == 1)
                            {
                                msu100_2_dat[0].push_back(line_buffer_raw1[x * 3 + 0] << 6);
                                msu100_2_dat[1].push_back(line_buffer_raw1[x * 3 + 1] << 6);
                                msu100_2_dat[2].push_back(line_buffer_raw2[x * 3 + 0] << 6);

                                msu100_2_dat[0].push_back(line_buffer_raw2[x * 3 + 1] << 6);
                                msu100_2_dat[1].push_back(line_buffer_raw2[x * 3 + 2] << 6);
                                msu100_2_dat[2].push_back(line_buffer_raw1[x * 3 + 2] << 6);
                            }
                        }
                    }

                    kmss_lines++;

                    progress = data_in.tellg();
                    if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                    {
                        lastTime = time(NULL);
                        logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                    }
                }

                data_in.close();

                // Products dataset
                satdump::products::DataSet dataset;
                dataset.satellite_name = sat_name;
                dataset.timestamp = satdump::get_median(timestamps);

                // KMSS1
                {
                    ////////////////////////////////////////// mtvza_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/KMSS_MSU100_1";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directory(directory);

                    logger->info("----------- KMSS MSU-100 1");
                    logger->info("Lines : " + std::to_string(kmss_lines));

                    satdump::products::ImageProduct kmss_product;
                    kmss_product.instrument_name = "kmss_msu100";
                    kmss_product.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-2_kmss_msu100_1.json")),
                                                             satdump::general_tle_registry->get_from_norad(norad), timestamps);

                    std::vector<satdump::ChannelTransform> transforms_def = {satdump::ChannelTransform().init_none(), satdump::ChannelTransform().init_none(), satdump::ChannelTransform().init_none()};

                    if (sat_name == "METEOR-M2-4")
                        transforms_def = {satdump::ChannelTransform().init_none(),                                     //
                                          satdump::ChannelTransform().init_affine_slantx(1, 1, 4, -3.8, 4000, 0.0012), //
                                          satdump::ChannelTransform().init_affine_slantx(1, 1, -2, -2, 4000, 0.0005)};

                    for (int i = 0; i < 3; i++)
                    {
                        auto img = image::Image(msu100_1_dat[i].data(), 16, 8000, kmss_lines, 1);
                        msu100_1_dat[i].clear();
                        kmss_product.images.push_back({i, "MSU100-" + std::to_string(i + 1), std::to_string(i + 1), img, 10, transforms_def[i]});
                    }

                    kmss_product.save(directory);
                    dataset.products_list.push_back("KMSS_MSU100_1");

                    ////////////////////////////////////////////////     mtvza_status = DONE;
                }

                // KMSS2
                {
                    ////////////////////////////////////////// mtvza_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/KMSS_MSU100_2";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directory(directory);

                    logger->info("----------- KMSS MSU-100 2");
                    logger->info("Lines : " + std::to_string(kmss_lines));

                    satdump::products::ImageProduct kmss_product;
                    kmss_product.instrument_name = "kmss_msu100";
                    kmss_product.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-2_kmss_msu100_2.json")),
                                                             satdump::general_tle_registry->get_from_norad(norad), timestamps);

                    std::vector<satdump::ChannelTransform> transforms_def = {satdump::ChannelTransform().init_none(), satdump::ChannelTransform().init_none(), satdump::ChannelTransform().init_none()};

                    if (sat_name == "METEOR-M2-4")
                        transforms_def = {satdump::ChannelTransform().init_none(),                                                          //
                                          satdump::ChannelTransform().init_affine_slantx(0.999800, 1.000000, -12.000000, -3.0, 0, -0.0009), //
                                          satdump::ChannelTransform().init_affine_slantx(0.999800, 1, -22, 5.700000, 0, 0.0005)};

                    for (int i = 0; i < 3; i++)
                    {
                        auto img = image::Image(msu100_2_dat[i].data(), 16, 8000, kmss_lines, 1);
                        msu100_2_dat[i].clear();
                        kmss_product.images.push_back({i, "MSU100-" + std::to_string(i + 1), std::to_string(i + 1), img, 10, transforms_def[i]});
                    }

                    kmss_product.save(directory);
                    dataset.products_list.push_back("KMSS_MSU100_2");

                    ////////////////////////////////////////////////     mtvza_status = DONE;
                }

                dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
            }
        }

        void MeteorXBandInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("METEOR X-Band Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##meteordumpinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Lines / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                if (d_instrument_mode == DUMP_TYPE_MTVZA)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MTVZA");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mtvza_lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mtvza_status);
                }

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

            ImGui::End();
        }

        std::string MeteorXBandInstrumentsDecoderModule::getID() { return "meteor_xband_instruments"; }

        std::shared_ptr<ProcessingModule> MeteorXBandInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MeteorXBandInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace meteor
