#include "formats.h"
#include "products/image_products.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "resources.h"
#include "metop_nat.h"

namespace nat2pro
{
    void decodeMHSNat(std::vector<uint8_t> &nat_file, std::string pro_output_file)
    {
        int image_width = 90;
        std::vector<uint16_t> mhs_data[5];
        int number_of_lines = 0;
        int all_gcps = 0;
        nlohmann::json gcps_all;

        std::vector<double> wavenumbers(5);

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
                for (int c = 0; c < 5; c++)
                    mhs_data[c].resize(number_of_lines * image_width);

                for (int c = 0; c < 5; c++)
                {
                    for (int i = 0; i < image_width; i++)
                    {
                        int32_t val;
                        ((uint8_t *)&val)[3] = nat_file[current_ptr + 83 + (i * 5 + c) * 4 + 0];
                        ((uint8_t *)&val)[2] = nat_file[current_ptr + 83 + (i * 5 + c) * 4 + 1];
                        ((uint8_t *)&val)[1] = nat_file[current_ptr + 83 + (i * 5 + c) * 4 + 2];
                        ((uint8_t *)&val)[0] = nat_file[current_ptr + 83 + (i * 5 + c) * 4 + 3];
                        // logger->info("%d", val);
                        mhs_data[c][(number_of_lines - 1) * image_width + (89 - i)] = val >> 4;
                    }
                }

                if ((number_of_lines - 1) % 4 == 0)
                {
                    double lat_first = get_deg_latlon(&nat_file[current_ptr + 3318 + 0]);
                    double lon_first = get_deg_latlon(&nat_file[current_ptr + 3318 + 4]);

                    double lat_last = get_deg_latlon(&nat_file[current_ptr + 4030 + 0]);
                    double lon_last = get_deg_latlon(&nat_file[current_ptr + 4030 + 4]);

                    gcps_all[all_gcps]["x"] = image_width - 1;
                    gcps_all[all_gcps]["y"] = number_of_lines - 1;
                    gcps_all[all_gcps]["lat"] = lat_first;
                    gcps_all[all_gcps]["lon"] = lon_first;
                    all_gcps++;

                    gcps_all[all_gcps]["x"] = 0;
                    gcps_all[all_gcps]["y"] = number_of_lines - 1;
                    gcps_all[all_gcps]["lat"] = lat_last;
                    gcps_all[all_gcps]["lon"] = lon_last;
                    all_gcps++;

                    for (int i = 1; i < 90; i += 4)
                    {
                        int px = i;
                        if (px >= 90)
                            continue;
                        double lat = get_deg_latlon(&nat_file[current_ptr + 3318 + i * 8 + 0]);
                        double lon = get_deg_latlon(&nat_file[current_ptr + 3318 + i * 8 + 4]);
                        gcps_all[all_gcps]["x"] = 89 - px;
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
            satdump::ImageProducts mhs_products;
            mhs_products.instrument_name = "mhs";
            // mhs_products.has_timestamps = true;
            // mhs_products.set_tle(satellite_tle);
            mhs_products.bit_depth = 16;
            // mhs_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
            // mhs_products.set_timestamps(avhrr_reader.timestamps);
            // mhs_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/metop_abc_avhrr.json")));

            nlohmann::json proj_cfg;
            proj_cfg["type"] = "normal_gcps";
            proj_cfg["gcp_cnt"] = all_gcps;
            proj_cfg["gcps"] = gcps_all;
            mhs_products.set_proj_cfg(proj_cfg);

            for (int i = 0; i < 5; i++)
                mhs_products.images.push_back({"MHS-" + std::to_string(i + 1), std::to_string(i + 1), image::Image(mhs_data[i].data(), 16, image_width, number_of_lines, 1)});

            // calibration
            nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/MHS.json"));

            // calib
            nlohmann::json calib_cfg;
            calib_cfg["calibrator"] = "metop_mhs_nat";
            calib_cfg["wavenumbers"] = calib_coefs["MetOp-B"]["wavenumber"];
            mhs_products.set_calibration(calib_cfg);
            for (int n = 0; n < 5; n++)
            {
                mhs_products.set_calibration_type(n, mhs_products.CALIB_RADIANCE);
                mhs_products.set_calibration_default_radiance_range(n, calib_coefs["all"]["default_display_range"][n][0].get<double>(), calib_coefs["all"]["default_display_range"][n][1].get<double>());
            }

            if (!std::filesystem::exists(pro_output_file))
                std::filesystem::create_directories(pro_output_file);
            mhs_products.save(pro_output_file);
        }
    }
}