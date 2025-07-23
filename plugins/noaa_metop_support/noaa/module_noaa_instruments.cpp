#include "module_noaa_instruments.h"
#include "common/repack.h"
#include "common/tracking/tle.h"
#include "common/utils.h"
#include "core/resources.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include "products/punctiform_product.h"
#include "utils/stats.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace noaa
{
    namespace instruments
    {
        NOAAInstrumentsDecoderModule::NOAAInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), is_gac(parameters.count("gac_mode") > 0 ? parameters["gac_mode"].get<bool>() : 0),
              is_dsb(parameters.count("dsb_mode") > 0 ? parameters["dsb_mode"].get<bool>() : 0),
              avhrr_reader(is_gac, parameters.count("year_override") > 0 ? parameters["year_override"].get<int>() : -1),
              hirs_reader(parameters.count("year_override") > 0 ? parameters["year_override"].get<int>() : -1),
              sem_reader(parameters.count("year_override") > 0 ? parameters["year_override"].get<int>() : -1),
              telemetry_reader(parameters.count("year_override") > 0 ? parameters["year_override"].get<int>() : -1)
        {
            fsfsm_enable_output = false;
        }

        void NOAAInstrumentsDecoderModule::process()
        {
            std::vector<int> spacecraft_ids;

            if (!is_dsb)
            {
                uint16_t buffer[11090] = {0};

                while (should_run())
                {
                    if (!is_gac)
                    {
                        read_data((uint8_t *)&buffer[0], 11090 * 2);
                        avhrr_reader.work_noaa(buffer); // avhrr

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
                                        sem_reader.work(frameBuffer);
                                        telemetry_reader.work(frameBuffer);

                                        // push the timestamps to AIP, since it depends on TIP for that (ridiculous)
                                        amsu_reader.last_TIP_timestamp = hirs_reader.last_timestamp;
                                    }
                                    else
                                    {
                                        amsu_reader.work_noaa(frameBuffer);
                                        mhs_reader.work_NOAA(frameBuffer);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        uint8_t rawWords[4159];
                        read_data((uint8_t *)rawWords, 4159);
                        repackBytesTo10bits(rawWords, 4159, buffer);

                        avhrr_reader.work_noaa(buffer); // avhrr

                        { // getting the TIP/AIP out
                            uint8_t frameBuffer[104];
                            for (int i = 0; i < 5; i++)
                            {
                                for (int j = 0; j < 104; j++)
                                {
                                    frameBuffer[j] = buffer[104 * (i + 1) + j - 1] >> 2;
                                }
                                hirs_reader.work(frameBuffer);
                                sem_reader.work(frameBuffer);
                                telemetry_reader.work(frameBuffer);
                                // push the timestamps to AIP, since it depends on TIP for that (ridiculous)
                                amsu_reader.last_TIP_timestamp = hirs_reader.last_timestamp;

                                for (int j = 0; j < 104; j++)
                                {
                                    frameBuffer[j] = buffer[104 * (i + 6) + j - 1] >> 2;
                                }
                                amsu_reader.work_noaa(frameBuffer);
                                mhs_reader.work_NOAA(frameBuffer);
                            }
                        }
                    }
                    spacecraft_ids.push_back(((buffer[6] & 0x078) >> 3) & 0x000F);
                }

                // done with the frames
                cleanup();

                int scid = satdump::most_common(spacecraft_ids.begin(), spacecraft_ids.end(), 0);
                spacecraft_ids.clear();
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

                // Products dataset
                satdump::products::DataSet dataset;
                dataset.satellite_name = sat_name;
                dataset.timestamp = satdump::get_median(avhrr_reader.timestamps);

                std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry->get_from_norad_time(norad, dataset.timestamp);

                // SATELLITE ID
                {
                    logger->info("----------- Satellite");
                    logger->info("NORAD : " + std::to_string(norad));
                    logger->info("Name  : " + sat_name);
                }

                // WARNINGS
                {
                    if (scid == 13)
                        logger->warn("NOAA-18 detected. MHS data will not be saved!");
                    if (scid == 7)
                        logger->warn("NOAA-15 detected. No MHS available!");
                    if (scid == 7)
                    {
                        time_t noaa15_age = dataset.timestamp - 895074720;
                        int seconds = noaa15_age % 60;
                        int minutes = (noaa15_age % 3600) / 60;
                        int hours = (noaa15_age % 86400) / 3600;
                        int days = noaa15_age / 86400;
                        logger->warn("Congratulations for receiving NOAA 15 on HRPT! It has been %d days, %d hours, %d minutes and %d seconds since it has been launched.", days, hours, minutes,
                                     seconds);
                        if (dataset.timestamp > 0)
                        {
                            time_t tttime = dataset.timestamp;
                            std::tm *timeReadable = gmtime(&tttime);
                            if (timeReadable->tm_mday == 13 && timeReadable->tm_mon == 4)
                                logger->critical("Happy birthday NOAA 15! You are now %d years old", timeReadable->tm_year + 1900 - 1998 + 1);
                        }
                    }
                }

                // AVHRR
                {
                    avhrr_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AVHRR";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directories(directory);

                    logger->info("----------- AVHRR/3");
                    logger->info("Lines : " + std::to_string(avhrr_reader.lines));

                    // product generation
                    satdump::products::ImageProduct avhrr_products;
                    avhrr_products.instrument_name = "avhrr_3";

                    std::string names[6] = {"1", "2", "3a", "3b", "4", "5"};

                    // calibration / images
                    nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/AVHRR.json"));
                    if (calib_coefs.contains(sat_name))
                    {
                        avhrr_reader.calibrate(calib_coefs[sat_name]);
                        avhrr_products.set_calibration("noaa_avhrr3", avhrr_reader.calib_out);
                        for (int i = 0; i < 6; i++)
                        {
                            avhrr_products.images.push_back({i, "AVHRR-" + names[i], names[i], avhrr_reader.getChannel(i), 10});
                            avhrr_products.set_channel_unit(i, i < 3 ? CALIBRATION_ID_REFLECTIVE_RADIANCE : CALIBRATION_ID_EMISSIVE_RADIANCE);
                            avhrr_products.set_channel_wavenumber(i, calib_coefs[sat_name]["channels"][i]["wavnb"]);
                        }
                    }
                    else
                        logger->warn("(AVHRR) Calibration data for " + sat_name + " not found. Calibration will not be performed");

                    nlohmann::ordered_json proj_settings;

                    if (scid == 7) // NOAA-15
                        proj_settings = loadJsonFile(resources::getResourcePath("projections_settings/noaa_15_avhrr.json"));
                    if (scid == 13) // NOAA-18
                        proj_settings = loadJsonFile(resources::getResourcePath("projections_settings/noaa_18_avhrr.json"));
                    if (scid == 15) // NOAA-19
                        proj_settings = loadJsonFile(resources::getResourcePath("projections_settings/noaa_19_avhrr.json"));

                    proj_settings.erase("apt_marker_offset");
                    if (is_gac)
                        proj_settings["image_width"] = 409;

                    avhrr_products.set_proj_cfg_tle_timestamps(proj_settings, satellite_tle, avhrr_reader.timestamps);

                    avhrr_products.save(directory);
                    dataset.products_list.push_back("AVHRR");

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

                    if (hirs_reader.sync < hirs_reader.line * 28)
                        logger->error("(HIRS) Possible filter wheel synchronization loss detected! Radiometric data may be invalid.");

                    satdump::products::ImageProduct hirs_products;
                    hirs_products.instrument_name = "hirs";
                    hirs_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/noaa_hirs.json")), satellite_tle, hirs_reader.timestamps);

                    for (int i = 0; i < 20; i++)
                        hirs_products.images.push_back({i, "HIRS-" + std::to_string(i + 1), std::to_string(i + 1), hirs_reader.getChannel(i), 13});

                    nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/HIRS.json"));
                    if (calib_coefs.contains(sat_name))
                    {
                        hirs_reader.calibrate(calib_coefs[sat_name], sat_name == "NOAA-15");
                        hirs_products.set_calibration("noaa_hirs", hirs_reader.calib_out);
                        for (int n = 0; n < 19; n++)
                        {
                            hirs_products.set_channel_unit(n, CALIBRATION_ID_EMISSIVE_RADIANCE);
                            hirs_products.set_channel_wavenumber(n, calib_coefs[sat_name]["wavenumber"][n].get<double>());
                        }
                        hirs_products.set_channel_unit(19, CALIBRATION_ID_REFLECTIVE_RADIANCE);
                    }
                    else
                        logger->warn("(HIRS) Calibration data for " + sat_name + " not found. Calibration will not be performed");

                    hirs_products.save(directory);
                    dataset.products_list.push_back("HIRS");

                    hirs_status = DONE;
                }

                // SEM
                {
                    sem_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SEM";
                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directory(directory);

                    logger->info("----------- SEM");
                    logger->info("Sample counts from selected channels :");
                    logger->info("Channel OP1   : " + std::to_string(sem_reader.getChannel(0).size()));
                    logger->info("Channel P8    : " + std::to_string(sem_reader.getChannel(20).size()));
                    logger->info("Channel 0DE1  : " + std::to_string(sem_reader.getChannel(22).size()));
                    logger->info("Channel 0EFL  : " + std::to_string(sem_reader.getChannel(38).size()));
                    logger->info("Backgrounds   : " + std::to_string(sem_reader.getChannel(54).size()));

                    satdump::products::PunctiformProduct sem_products;
                    sem_products.instrument_name = "sem";
                    sem_products.set_tle(satellite_tle);
                    for (int i = 0; i < 62; i++)
                    {
                        satdump::products::PunctiformProduct::DataHolder h;
                        h.channel_name = sem_reader.channel_names[i];
                        h.timestamps = sem_reader.getTimestamps(i);
                        auto v = sem_reader.getChannel(i);
                        for (int x = 0; x < v.size(); x++)
                            h.data.push_back(v[x]);
                        sem_products.data.push_back(h);
                    }

                    sem_products.save(directory);
                    dataset.products_list.push_back("SEM");
                    sem_status = DONE;
                }

                // telemetry
                {
                    telemetry_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/telemetry";
                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directory(directory);

                    saveJsonFile(directory + "/telem.json", telemetry_reader.dump_telemetry());
                    telemetry_status = DONE;
                }

                // MHS
                if (scid == 15)
                { // only N19 has operational MHS
                    mhs_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MHS";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directories(directory);

                    logger->info("----------- MHS");
                    logger->info("Lines : " + std::to_string(mhs_reader.line));

                    satdump::products::ImageProduct mhs_products;
                    mhs_products.instrument_name = "mhs";

                    for (int i = 0; i < 5; i++)
                        mhs_products.images.push_back({i, "MHS-" + std::to_string(i + 1), std::to_string(i + 1), mhs_reader.getChannel(i), 16});

                    mhs_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/noaa_19_mhs.json")), satellite_tle, mhs_reader.timestamps);

                    // calibration
                    nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/MHS.json"));
                    if (calib_coefs.contains(sat_name))
                    {
                        mhs_reader.calibrate(calib_coefs[sat_name]);
                        mhs_products.set_calibration("noaa_mhs", mhs_reader.calib_out);
                        for (int i = 0; i < 5; i++)
                        {
                            mhs_products.set_channel_unit(i, CALIBRATION_ID_EMISSIVE_RADIANCE);
                            mhs_products.set_channel_wavenumber(i, calib_coefs[sat_name]["wavenumber"][i]);
                        }
                    }
                    else
                        logger->warn("(MHS) Calibration data for " + sat_name + " not found. Calibration will not be performed");

                    saveJsonFile(directory + "/MHS_tlm.json", mhs_reader.dump_telemetry(calib_coefs[sat_name]));
                    mhs_products.save(directory);
                    dataset.products_list.push_back("MHS");

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

                    amsu_reader.correlate();

                    satdump::products::ImageProduct amsu_products;
                    amsu_products.instrument_name = "amsu_a";
                    amsu_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/noaa_amsu.json")), satellite_tle, amsu_reader.common_timestamps);

                    for (int i = 0; i < 15; i++)
                        amsu_products.images.push_back({i, "AMSU-A-" + std::to_string(i + 1), std::to_string(i + 1), amsu_reader.getChannel(i), 16});

                    // calibration
                    nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/AMSU-A.json"));
                    if (calib_coefs.contains(sat_name))
                    {
                        calib_coefs[sat_name]["all"] = calib_coefs["all"];
                        amsu_reader.calibrate(calib_coefs[sat_name]);
                        amsu_products.set_calibration("noaa_amsu", amsu_reader.calib_out);
                        for (int i = 0; i < 15; i++)
                        {
                            amsu_products.set_channel_unit(i, CALIBRATION_ID_EMISSIVE_RADIANCE);
                            amsu_products.set_channel_wavenumber(i, calib_coefs["all"]["wavenumber"][i]);
                        }
                    }
                    else
                        logger->warn("(AMSU) Calibration data for " + sat_name + " not found. Calibration will not be performed");

                    amsu_products.save(directory);
                    dataset.products_list.push_back("AMSU");

                    amsu_status = DONE;
                }

                dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
            }
            else
            {
                std::vector<uint8_t> scid_list;
                uint8_t buffer[104];
                while (should_run())
                {
                    read_data((uint8_t *)&buffer[0], 104);
                    hirs_reader.work(buffer);
                    sem_reader.work(buffer);
                    telemetry_reader.work(buffer);
                    scid_list.push_back(buffer[2] & 0b00001111);
                }

                // ID
                uint8_t scid = satdump::most_common(scid_list.begin(), scid_list.end(), 0);
                scid_list.clear();
                int norad = 0;
                std::string sat_name = "Unknown NOAA";
                if (scid == 8)
                { // N15
                    norad = 25338;
                    sat_name = "NOAA-15";
                }
                else if (scid == 13 || scid == 14)
                { // N18
                    norad = 28654;
                    sat_name = "NOAA-18";
                }
                else if (scid == 15 || scid == 0)
                { // N19
                    norad = 33591;
                    sat_name = "NOAA-19";
                }

                // Products dataset
                satdump::products::DataSet dataset;
                dataset.satellite_name = sat_name;
                dataset.timestamp = satdump::get_median(hirs_reader.timestamps);

                std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry->get_from_norad_time(norad, dataset.timestamp);

                // SATELLITE ID
                {
                    logger->info("----------- Satellite");
                    logger->info("NORAD : " + std::to_string(norad));
                    logger->info("Name  : " + sat_name);
                }

                // HIRS
                /*
                {
                    hirs_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/HIRS";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directories(directory);

                    logger->info("----------- HIRS");
                    logger->info("Lines : " + std::to_string(hirs_reader.line));

                    satdump::products::ImageProduct hirs_products;
                    hirs_products.instrument_name = "hirs";
                    hirs_products.has_timestamps = true;
                    hirs_products.bit_depth = 13;
                    hirs_products.set_tle(satellite_tle);
                    hirs_products.timestamp_type = satdump::ImageProduct::TIMESTAMP_LINE;
                    hirs_products.set_timestamps(hirs_reader.timestamps);
                    hirs_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/noaa_hirs.json")));

                    for (int i = 0; i < 20; i++)
                        hirs_products.images.push_back({"HIRS-" + std::to_string(i + 1), std::to_string(i + 1), hirs_reader.getChannel(i)});

                    hirs_products.save(directory);
                    dataset.products_list.push_back("HIRS");

                    hirs_status = DONE;
                }
                */
                {
                    hirs_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/HIRS";

                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directories(directory);

                    logger->info("----------- HIRS");
                    logger->info("Lines : " + std::to_string(hirs_reader.line));

                    if (hirs_reader.sync < hirs_reader.line * 28)
                        logger->error("(HIRS) Possible filter wheel synchronization loss detected! Radiometric data may be invalid.");

                    satdump::products::ImageProduct hirs_products;
                    hirs_products.instrument_name = "hirs";
                    hirs_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/noaa_hirs.json")), satellite_tle, hirs_reader.timestamps);

                    for (int i = 0; i < 20; i++)
                        hirs_products.images.push_back({i, "HIRS-" + std::to_string(i + 1), std::to_string(i + 1), hirs_reader.getChannel(i), 13});

                    nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/HIRS.json"));
                    if (calib_coefs.contains(sat_name))
                    {
                        hirs_reader.calibrate(calib_coefs[sat_name], sat_name == "NOAA-15");
                        hirs_products.set_calibration("noaa_hirs", hirs_reader.calib_out);
                        for (int n = 0; n < 19; n++)
                        {
                            hirs_products.set_channel_unit(n, CALIBRATION_ID_EMISSIVE_RADIANCE);
                            hirs_products.set_channel_wavenumber(n, calib_coefs[sat_name]["wavenumber"][n].get<double>());
                        }
                        hirs_products.set_channel_unit(19, CALIBRATION_ID_REFLECTIVE_RADIANCE);
                        for (int n = 0; n < 20; n++)
                            hirs_products.set_channel_wavenumber(n, hirs_reader.calib_out["wavenumbers"][n]);
                    }
                    else
                        logger->warn("(HIRS) Calibration data for " + sat_name + " not found. Calibration will not be performed");

                    hirs_products.save(directory);
                    dataset.products_list.push_back("HIRS");

                    hirs_status = DONE;
                }

                /* // SEM
                 {
                     sem_status = SAVING;
                     std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SEM";
                     if (!std::filesystem::exists(directory))
                         std::filesystem::create_directory(directory);

                     logger->info("----------- SEM");
                     logger->info("Sample counts from selected channels :");
                     logger->info("Channel OP1   : " + std::to_string(sem_reader.getChannel(0).size()));
                     logger->info("Channel P8    : " + std::to_string(sem_reader.getChannel(20).size()));
                     logger->info("Channel 0DE1  : " + std::to_string(sem_reader.getChannel(22).size()));
                     logger->info("Channel 0EFL  : " + std::to_string(sem_reader.getChannel(38).size()));
                     logger->info("Backgrounds   : " + std::to_string(sem_reader.getChannel(54).size()));

                     satdump::RadiationProducts sem_products;
                     sem_products.instrument_name = "sem";
                     sem_products.set_tle(satellite_tle);
                     for (int i = 0; i < 62; i++)
                     {
                         sem_products.channel_counts.push_back(sem_reader.getChannel(i));
                         sem_products.set_timestamps(i, sem_reader.getTimestamps(i));
                         sem_products.set_channel_name(i, sem_reader.channel_names[i]);
                     }

                     sem_products.save(directory);
                     dataset.products_list.push_back("SEM");
                     sem_status = DONE;
                 } TODOREWORK */

                // telemetry
                {
                    telemetry_status = SAVING;
                    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/telemetry";
                    if (!std::filesystem::exists(directory))
                        std::filesystem::create_directory(directory);

                    saveJsonFile(directory + "/telem.json", telemetry_reader.dump_telemetry());
                    telemetry_status = DONE;
                }
                dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
            }
        }

        void NOAAInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("NOAA Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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
                ImGui::TextColored(style::theme.green, "%d", avhrr_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(avhrr_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("HIRS");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", hirs_reader.line);
                ImGui::TableSetColumnIndex(2);
                drawStatus(hirs_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MHS");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", mhs_reader.line);
                ImGui::TableSetColumnIndex(2);
                drawStatus(mhs_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AMSU A1");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", amsu_reader.linesA1);
                ImGui::TableSetColumnIndex(2);
                drawStatus(amsu_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AMSU A2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", amsu_reader.linesA2);
                ImGui::TableSetColumnIndex(2);
                drawStatus(amsu_status);

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string NOAAInstrumentsDecoderModule::getID() { return "noaa_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> NOAAInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<NOAAInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace noaa
