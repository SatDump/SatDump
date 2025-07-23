#include "formats.h"
#include "products/image_product.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "core/resources.h"
#include "metop_nat.h"

#include "image/processing.h"

#include "utils/stats.h"
#include "metop_helper.h"

namespace nat2pro
{
    void decodeIASINat(std::vector<uint8_t> &nat_file, std::string pro_output_file)
    {
        int image_width = 60;
        std::vector<uint16_t> iasi_data[8700];
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

            if (main_header.record_class == 8 && main_header.record_subclass == 2)
            {
                logger->info("MDR 1C!");

                number_of_lines += 2;
                for (int c = 0; c < 8700; c++)
                    iasi_data[c].resize(number_of_lines * image_width);

                for (int c = 0; c < 8700; c++)
                {
                    for (int i = 0; i < image_width / 2; i++)
                    {
                        int pos4 = (i * 4 + 0) * 8700 + c; // c * 30 * 4 + 0 * 30 + i;
                        int pos2 = (i * 4 + 1) * 8700 + c; // c * 30 * 4 + 1 * 30 + i;
                        int pos3 = (i * 4 + 2) * 8700 + c; // c * 30 * 4 + 2 * 30 + i;
                        int pos1 = (i * 4 + 3) * 8700 + c; // c * 30 * 4 + 3 * 30 + i;

                        uint16_t val1;
                        ((uint8_t *)&val1)[1] = nat_file[current_ptr + 276790 + pos1 * 2 + 0];
                        ((uint8_t *)&val1)[0] = nat_file[current_ptr + 276790 + pos1 * 2 + 1];

                        uint16_t val2;
                        ((uint8_t *)&val2)[1] = nat_file[current_ptr + 276790 + pos2 * 2 + 0];
                        ((uint8_t *)&val2)[0] = nat_file[current_ptr + 276790 + pos2 * 2 + 1];

                        uint16_t val3;
                        ((uint8_t *)&val3)[1] = nat_file[current_ptr + 276790 + pos3 * 2 + 0];
                        ((uint8_t *)&val3)[0] = nat_file[current_ptr + 276790 + pos3 * 2 + 1];

                        uint16_t val4;
                        ((uint8_t *)&val4)[1] = nat_file[current_ptr + 276790 + pos1 * 2 + 0];
                        ((uint8_t *)&val4)[0] = nat_file[current_ptr + 276790 + pos4 * 2 + 1];

                        // logger->info("%d", val);
                        iasi_data[c][(number_of_lines - 2) * image_width + 59 - (i * 2 + 0)] = val1;
                        iasi_data[c][(number_of_lines - 2) * image_width + 59 - (i * 2 + 1)] = val2;
                        iasi_data[c][(number_of_lines - 1) * image_width + 59 - (i * 2 + 0)] = val3;
                        iasi_data[c][(number_of_lines - 1) * image_width + 59 - (i * 2 + 1)] = val4;
                    }
                }

                timestamps.push_back(main_header.record_start_time);

                // if ((number_of_lines - 2) % 4 == 0)
                {
                    for (int i = 1; i < 30; i += 1)
                    {
                        int px = i;
                        if (px >= 30)
                            continue;
                        double lat = get_deg_latlon(&nat_file[current_ptr + 255893 + (i * 4) * 8 + 4]) / 100.0;
                        double lon = get_deg_latlon(&nat_file[current_ptr + 255893 + (i * 4) * 8 + 0]) / 100.0;
                        gcps_all[all_gcps]["x"] = 59 - px * 2;
                        gcps_all[all_gcps]["y"] = number_of_lines - 2;
                        gcps_all[all_gcps]["lat"] = lat;
                        gcps_all[all_gcps]["lon"] = lon;
                        all_gcps++;

                        logger->trace("%f %f", lon, lat);
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

            satdump::products::ImageProduct iasi_products;
            iasi_products.instrument_name = "iasi";
            iasi_products.save_as_matrix = true;

            iasi_products.set_product_source(info.sat_name);
            iasi_products.set_product_timestamp(ptime);

            auto p = loadJsonFile(resources::getResourcePath("projections_settings/metop_abc_iasi.json"));
#if 0
            iasi_products.set_proj_cfg_tle_timestamps(p, info.satellite_tle, timestamps);
#else
            nlohmann::json proj_cfg = p;
            proj_cfg["type"] = "gcps_timestamps_line";
            proj_cfg["gcp_cnt"] = all_gcps;
            proj_cfg["gcps"] = gcps_all;
            proj_cfg["timestamps"] = timestamps;
            iasi_products.set_proj_cfg(proj_cfg);
            // TODOREWORK switch to GCPs again!
#endif
            logger->critical("TODOREWORK switch to GCPs again!");

            for (int i = 0; i < 8700; i++)
                iasi_products.images.push_back({i, "IASI", std::to_string(i + 1), image::Image(iasi_data[i].data(), 16, image_width, number_of_lines, 1), 16});

            // for (int i = 0; i < 8700; i++)
            //     image::equalize(iasi_products.images[i].image);

            /*  // calib
              nlohmann::json calib_cfg;
              calib_cfg["calibrator"] = "metop_amsu_nat";
              nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/AMSU-A.json"));
              calib_cfg["wavenumbers"] = calib_coefs["all"]["wavenumber"];

              iasi_products.set_calibration(calib_cfg);

              for (int c = 0; c < 15; c++)
              {
                  iasi_products.set_calibration_type(c, iasi_products.CALIB_RADIANCE);
                  iasi_products.set_calibration_default_radiance_range(c, calib_coefs["all"]["default_display_range"][c][0].get<double>(), calib_coefs["all"]["default_display_range"][c][1].get<double>());
              }
  */

            // save
            if (!std::filesystem::exists(pro_output_file))
                std::filesystem::create_directories(pro_output_file);
            iasi_products.save(pro_output_file);
        }
    }
}