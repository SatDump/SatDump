#include "formats.h"
#include "products/image_product.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "core/resources.h"
#include "metop_nat.h"

#include "utils/stats.h"
#include "metop_helper.h"

namespace nat2pro
{
    void decodeGOMENat(std::vector<uint8_t> &nat_file, std::string pro_output_file)
    {
        logger->error("EUMETSAT's docs are broken.");
        return;

        int image_width = 32;
        std::vector<uint16_t> gome_data[6144];
        int number_of_lines = 0;
        int all_gcps = 0;
        nlohmann::json gcps_all;

        std::vector<double> timestamps;

        std::string sat_id = "M00";

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
                sat_id = full_header.substr(664 - 20 + 32, 3);
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

            if (main_header.record_class == 8 && main_header.record_subclass == 6)
            {
                logger->info("MDR 1B!");

                for (int i = 0; i < 6; i++)
                {
                    int val = nat_file[current_ptr + 98636 + i * 2 + 1] << 8 | nat_file[current_ptr + 98636 + i * 2 + 0];
                    printf("%d %d\n", i, val);
                }

                number_of_lines++;
                for (int c = 0; c < 15; c++)
                    gome_data[c].resize(number_of_lines * image_width);

                for (int c = 0; c < 15; c++)
                {
                    for (int i = 0; i < image_width; i++)
                    {
                        int32_t val;
                        ((uint8_t *)&val)[3] = nat_file[current_ptr + 22 + (i * 15 + c) * 4 + 0];
                        ((uint8_t *)&val)[2] = nat_file[current_ptr + 22 + (i * 15 + c) * 4 + 1];
                        ((uint8_t *)&val)[1] = nat_file[current_ptr + 22 + (i * 15 + c) * 4 + 2];
                        ((uint8_t *)&val)[0] = nat_file[current_ptr + 22 + (i * 15 + c) * 4 + 3];
                        // logger->info("%d", val);
                        gome_data[c][(number_of_lines - 1) * image_width + (29 - i)] = val >> 2;
                    }
                }

                timestamps.push_back(main_header.record_start_time);

                // if ((number_of_lines - 1) % 4 == 0)
                {
                    for (int i = 1; i < 30; i += 1)
                    {
                        int px = i;
                        if (px >= 30)
                            continue;
                        double lat = get_deg_latlon(&nat_file[current_ptr + 2082 + i * 8 + 0]);
                        double lon = get_deg_latlon(&nat_file[current_ptr + 2082 + i * 8 + 4]);
                        gcps_all[all_gcps]["x"] = 29 - px;
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
            auto ptime = satdump::get_median(timestamps);
            auto info = getMetOpSatInfoFromID(sat_id, ptime);

            satdump::products::ImageProduct gome_products;
            gome_products.instrument_name = "gome";
            gome_products.save_as_matrix = true;

            gome_products.set_product_source(info.sat_name);
            gome_products.set_product_timestamp(ptime);

            auto p = loadJsonFile(resources::getResourcePath("projections_settings/metop_abc_gome.json"));
#if 0
            gome_products.set_proj_cfg_tle_timestamps(p, info.satellite_tle, timestamps);
#else
            nlohmann::json proj_cfg = p;
            proj_cfg["type"] = "gcps_timestamps_line";
            proj_cfg["gcp_cnt"] = all_gcps;
            proj_cfg["gcps"] = gcps_all;
            proj_cfg["timestamps"] = timestamps;
            gome_products.set_proj_cfg(proj_cfg);
#endif

            for (int i = 0; i < 6144; i++)
                gome_products.images.push_back({i, "GOME", std::to_string(i + 1), image::Image(gome_data[i].data(), 16, image_width, number_of_lines, 1), 16});

            /*  // calib
              nlohmann::json calib_cfg;
              calib_cfg["calibrator"] = "metop_amsu_nat";
              nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/AMSU-A.json"));
              calib_cfg["wavenumbers"] = calib_coefs["all"]["wavenumber"];

              gome_products.set_calibration(calib_cfg);

              for (int c = 0; c < 15; c++)
              {
                  gome_products.set_calibration_type(c, gome_products.CALIB_RADIANCE);
                  gome_products.set_calibration_default_radiance_range(c, calib_coefs["all"]["default_display_range"][c][0].get<double>(), calib_coefs["all"]["default_display_range"][c][1].get<double>());
              }*/

            // save
            if (!std::filesystem::exists(pro_output_file))
                std::filesystem::create_directories(pro_output_file);
            gome_products.save(pro_output_file);
        }
    }
}