#include "formats.h"
#include "products/image_products.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "resources.h"
#include "metop_nat.h"

namespace nat2pro
{
    void decodeHIRSNat(std::vector<uint8_t> &nat_file, std::string pro_output_file)
    {
        int image_width = 56;
        std::vector<uint16_t> hirs_data[20];
        int number_of_lines = 0;
        int all_gcps = 0;
        nlohmann::json gcps_all;

        size_t current_ptr = 0;
        while (current_ptr < nat_file.size())
        {
            GenericRecordHeader main_header(&nat_file[current_ptr]);

            if (main_header.record_size == 0) //|| main_header.record_class == 8)
                break;

            if (main_header.record_class == 1)
            {
                std::string full_header(&nat_file[current_ptr + 20], &nat_file[current_ptr + 20] + main_header.record_size - 20);
                printf("%s\n", full_header.c_str());
            }

            /*if (main_header.record_class == 2)
            {
                uint64_t value_idk = nat_file[current_ptr + 69 + 0] << 32 |
                                     nat_file[current_ptr + 69 + 1] << 24 |
                                     nat_file[current_ptr + 69 + 2] << 16 |
                                     nat_file[current_ptr + 69 + 3] << 8 |
                                     nat_file[current_ptr + 69 + 4];

                printf("%d\n", value_idk);
            }*/

            if (main_header.record_class == 8 && main_header.record_subclass == 2)
            {
                logger->info("MDR 1B!");

                number_of_lines++;
                for (int c = 0; c < 20; c++)
                    hirs_data[c].resize(number_of_lines * image_width);

                for (int c = 0; c < 20; c++)
                {
                    for (int i = 0; i < image_width; i++)
                    {
                        int32_t val;
                        ((uint8_t *)&val)[3] = nat_file[current_ptr + 74 + (i * 84 + 4) + c * 4 + 0];
                        ((uint8_t *)&val)[2] = nat_file[current_ptr + 74 + (i * 84 + 4) + c * 4 + 1];
                        ((uint8_t *)&val)[1] = nat_file[current_ptr + 74 + (i * 84 + 4) + c * 4 + 2];
                        ((uint8_t *)&val)[0] = nat_file[current_ptr + 74 + (i * 84 + 4) + c * 4 + 3];
                        // logger->info("%d", val);
                        hirs_data[c][(number_of_lines - 1) * image_width + (55 - i)] = (val >> 16) + 32768;
                        if (c != 11)
                            hirs_data[c][(number_of_lines - 1) * image_width + (55 - i)] = 65535 - hirs_data[c][(number_of_lines - 1) * image_width + (55 - i)];
                    }
                }

                if ((number_of_lines - 1) % 4 == 0)
                {
                    for (int i = 1; i < 56; i += 1)
                    {
                        int px = i;
                        if (px >= 56)
                            continue;
                        double lat = get_deg_latlon(&nat_file[current_ptr + 5620 + i * 8 + 0]);
                        double lon = get_deg_latlon(&nat_file[current_ptr + 5620 + i * 8 + 4]);
                        gcps_all[all_gcps]["x"] = 55 - px;
                        gcps_all[all_gcps]["y"] = number_of_lines - 1;
                        gcps_all[all_gcps]["lat"] = lat;
                        gcps_all[all_gcps]["lon"] = lon;
                        all_gcps++;

                        // logger->trace("%f %f", lon, lat);
                    }

                    // logger->info("%d %d %d %f %f %f %f %d", (int)main_header.record_class, (int)main_header.record_size, (int)main_header.record_subclass_version,
                    //              lon_first, lat_first, lon_last, lat_last, ch3a);
                }
            }

            current_ptr += main_header.record_size;
        }

        {
            satdump::ImageProducts hirs_products;
            hirs_products.instrument_name = "hirs";
            // hirs_products.has_timestamps = true;
            // hirs_products.set_tle(satellite_tle);
            hirs_products.bit_depth = 16;
            // hirs_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
            // hirs_products.set_timestamps(avhrr_reader.timestamps);
            // hirs_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/metop_abc_avhrr.json")));

            nlohmann::json proj_cfg;
            proj_cfg["type"] = "normal_gcps";
            proj_cfg["gcp_cnt"] = all_gcps;
            proj_cfg["gcps"] = gcps_all;
            hirs_products.set_proj_cfg(proj_cfg);

            const int HIRSChannels[20] = {0, 16, 1, 2, 12, 3, 17, 10, 18, 6, 7, 19, 9, 13, 5, 4, 14, 11, 15, 8};
            for (int i = 0; i < 20; i++)
                hirs_products.images.push_back({"HIRS-" + std::to_string(HIRSChannels[i] + 1), std::to_string(HIRSChannels[i] + 1), image::Image(hirs_data[i].data(), 16, image_width, number_of_lines, 1)});

            // calib
            //    nlohmann::json calib_cfg;
            //    calib_cfg["calibrator"] = "metop_amsu_nat";
            //    nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/AMSU-A.json"));
            //    calib_cfg["wavenumbers"] = calib_coefs["all"]["wavenumber"];

            //    hirs_products.set_calibration(calib_cfg);

            //    for (int c = 0; c < 20; c++)
            //    {
            //        hirs_products.set_calibration_type(c, hirs_products.CALIB_RADIANCE);
            //        hirs_products.set_calibration_default_radiance_range(c, calib_coefs["all"]["default_display_range"][c][0].get<double>(), calib_coefs["all"]["default_display_range"][c][1].get<double>());
            //    }

            // save
            if (!std::filesystem::exists(pro_output_file))
                std::filesystem::create_directories(pro_output_file);
            hirs_products.save(pro_output_file);
        }
    }
}