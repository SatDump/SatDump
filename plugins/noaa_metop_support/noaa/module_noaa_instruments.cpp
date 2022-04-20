#include "module_noaa_instruments.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "products/image_products.h"

namespace noaa
{
    namespace instruments
    {
        NOAAInstrumentsDecoderModule::NOAAInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters),
              is_gac(parameters.count("gac_mode") > 0 ? parameters["gac_mode"].get<bool>() : 0),
              is_dsb(parameters.count("dsb_mode") > 0 ? parameters["dsb_mode"].get<bool>() : 0),
              avhrr_reader(is_gac)
        {
        }

        void NOAAInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            if (!is_dsb)
            {
                uint16_t buffer[11090] = {0};

                while (!data_in.eof())
                {
                    if (!is_gac)
                    {
                        data_in.read((char *)&buffer[0], 11090 * 2);
                        avhrr_reader.work(buffer); // avhrr

                        { // getting the TIP/AIP out
                            int frmnum = ((buffer[6] >> 7) & 0b00000010) | ((buffer[6] >> 7) & 1);
                            uint8_t frameBuffer[104];

                            if (frmnum == 1 || frmnum == 3)
                            {
                                for (int i = 0; i < 5; i++)
                                {
                                    for (int j = 0; j < 104; j++)
                                    {
                                        frameBuffer[j] = buffer[104 * (i + 1) + j - 1] >> 2;
                                    }
                                    if (frmnum == 1)
                                    {
                                        hirs_reader.work(frameBuffer);
                                    }
                                    else
                                    {
                                        amsu_reader.work(frameBuffer);
                                        mhs_reader.work_NOAA(frameBuffer);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        uint8_t rawWords[4159];
                        data_in.read((char *)rawWords, 4159);
                        repackBytesTo10bits(rawWords, 4159, buffer);

                        avhrr_reader.work(buffer); // avhrr

                        { // getting the TIP/AIP out
                            uint8_t frameBuffer[104];
                            for (int i = 0; i < 5; i++)
                            {
                                for (int j = 0; j < 104; j++)
                                {
                                    frameBuffer[j] = buffer[104 * (i + 1) + j - 1] >> 2;
                                }
                                hirs_reader.work(frameBuffer);
                                for (int j = 0; j < 104; j++)
                                {
                                    frameBuffer[j] = buffer[104 * (i + 6) + j - 1] >> 2;
                                }
                                amsu_reader.work(frameBuffer);
                                mhs_reader.work_NOAA(frameBuffer);
                            }
                        }
                    }
                } // done with the frames
                data_in.close();

                int scid = most_common(avhrr_reader.spacecraft_ids.begin(), avhrr_reader.spacecraft_ids.end());
                avhrr_reader.spacecraft_ids.clear();
                int norad = 0;
                std::string sat_name = "Unknown NOAA";
                if (scid == 7)
                { // N15
                    norad = 25338;
                    sat_name = "NOAA-15";
                }
                else if (scid == 13)
                { // N18
                    norad = 28654;
                    sat_name = "NOAA-18";
                }
                else if (scid == 15)
                { // N19
                    norad = 33591;
                    sat_name = "NOAA-19";
                }

                // SATELLITE ID
                {
                    logger->info("----------- Satellite");
                    logger->info("NORAD : " + std::to_string(norad));
                    logger->info("Name  : " + sat_name);
                }

                // AVHRR
                {
                    avhrr_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AVHRR";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directories(directory);

                    logger->info("----------- AVHRR/3");
                    logger->info("Lines : " + std::to_string(avhrr_reader.lines));

                    satdump::ImageProducts avhrr_products;
                    avhrr_products.instrument_name = "avhrr_3";
                    avhrr_products.has_timestamps = true;
                    avhrr_products.bit_depth = 10;
                    avhrr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                    avhrr_products.set_timestamps(avhrr_reader.timestamps);

                    for (int i = 0; i < 5; i++)
                        avhrr_products.images.push_back({"AVHRR-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), avhrr_reader.getChannel(i)});

                    avhrr_products.save(directory);

                    avhrr_status = DONE;
                }

                // HIRS
                {
                    hirs_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/HIRS";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directories(directory);

                    logger->info("----------- HIRS");
                    logger->info("Lines : " + std::to_string(hirs_reader.line));

                    satdump::ImageProducts hirs_products;
                    hirs_products.instrument_name = "hirs";
                    hirs_products.has_timestamps = false;
                    hirs_products.bit_depth = 13;
                    // hirs_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                    // hirs_products.set_timestamps(hirs_reader.timestamps);

                    for (int i = 0; i < 20; i++)
                        hirs_products.images.push_back({"HIRS-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), hirs_reader.getChannel(i)});

                    hirs_products.save(directory);

                    hirs_status = DONE;
                }

                // MHS
                {
                    mhs_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MHS";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directories(directory);

                    logger->info("----------- MHS");
                    logger->info("Lines : " + std::to_string(mhs_reader.line));

                    satdump::ImageProducts mhs_products;
                    mhs_products.instrument_name = "mhs";
                    mhs_products.has_timestamps = true;
                    mhs_products.bit_depth = 16;
                    mhs_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                    mhs_products.set_timestamps(mhs_reader.timestamps);
                    for (int i = 0; i < 5; i++)
                    {
                        mhs_products.set_calibration_polynomial_per_line(i, mhs_reader.calibration_coefs[i]);
                        //mhs_products.set_wavenumber(i, mhs::calibration::wavenumber[i]);
                    }

                    for (int i = 0; i < 5; i++)
                        mhs_products.images.push_back({"MHS-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), mhs_reader.getChannel(i)});

                    mhs_products.save(directory);

                    mhs_status = DONE;
                }

                // AMSU
                {
                    amsu_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AMSU";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directories(directory);

                    logger->info("----------- AMSU");
                    logger->info("Lines A1: " + std::to_string(amsu_reader.linesA1));
                    logger->info("Lines A2: " + std::to_string(amsu_reader.linesA2));

                    satdump::ImageProducts amsu_products;
                    amsu_products.instrument_name = "amsu_a";
                    amsu_products.has_timestamps = false;
                    amsu_products.bit_depth = 16;
                    // amsu_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                    // amsu_products.set_timestamps(amsu_reader.timestamps);

                    for (int i = 0; i < 15; i++)
                        amsu_products.images.push_back({"AMSU-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), amsu_reader.getChannel(i)});

                    amsu_products.save(directory);

                    amsu_status = DONE;
                }
            }
            else
            {
                uint8_t buffer[104];
                while (!data_in.eof())
                {
                    data_in.read((char *)&buffer[0], 104);
                    hirs_reader.work(buffer);
                }

                hirs_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/HIRS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directories(directory);

                logger->info("----------- HIRS");
                logger->info("Lines : " + std::to_string(hirs_reader.line));

                satdump::ImageProducts hirs_products;
                hirs_products.instrument_name = "hirs";
                hirs_products.has_timestamps = false;
                hirs_products.bit_depth = 13;
                // hirs_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                // hirs_products.set_timestamps(hirs_reader.timestamps);

                for (int i = 0; i < 20; i++)
                    hirs_products.images.push_back({"HIRS-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), hirs_reader.getChannel(i)});

                hirs_products.save(directory);

                hirs_status = DONE;
            }
        }

        void NOAAInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("NOAA Instruments Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##noaainstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
                ImGui::Text("AVHRR");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", avhrr_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(avhrr_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("HIRS");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", hirs_reader.line);
                ImGui::TableSetColumnIndex(2);
                drawStatus(hirs_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MHS");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", mhs_reader.line);
                ImGui::TableSetColumnIndex(2);
                drawStatus(mhs_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AMSU A1");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", amsu_reader.linesA1);
                ImGui::TableSetColumnIndex(2);
                drawStatus(amsu_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AMSU A2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", amsu_reader.linesA2);
                ImGui::TableSetColumnIndex(2);
                drawStatus(amsu_status);

                ImGui::EndTable();
            }

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string NOAAInstrumentsDecoderModule::getID()
        {
            return "noaa_instruments";
        }

        std::vector<std::string> NOAAInstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> NOAAInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<NOAAInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace noaa