#include "main.h"
#include "products/image_products.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "resources.h"

double get_deg_latlon(uint8_t *data)
{
    int32_t val;
    ((uint8_t *)&val)[3] = data[0];
    ((uint8_t *)&val)[2] = data[1];
    ((uint8_t *)&val)[1] = data[2];
    ((uint8_t *)&val)[0] = data[3];
    return double(val) / 1e4;
}

struct GenericRecordHeader
{
    uint8_t record_class;
    uint8_t instrument_group;
    uint8_t record_subclass;
    uint8_t record_subclass_version;
    uint32_t record_size;
    double record_start_time;
    double record_stop_time;

    GenericRecordHeader(uint8_t *data)
    {
        record_class = data[0];
        instrument_group = data[1];
        record_subclass = data[2];
        record_subclass_version = data[3];
        record_size = data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];
        // record_start_time = data[8] << 40 | data[9] << 32 | data[10] << 24 | data[11] << 16 | data[12] << 8 | data[13];
        // record_stop_time = data[14] << 40 | data[15] << 32 | data[16] << 24 | data[17] << 16 | data[18] << 8 | data[19];
    }
};

void decodeAVHRRNat(std::vector<uint8_t> nat_file, std::string pro_output_file)
{
    int image_width = 0;
    std::vector<uint16_t> avhrr_data[6];
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

        if (main_header.record_class == 8)
        {
            uint16_t number_of_samples =
                nat_file[current_ptr + 22 + 0] << 8 |
                nat_file[current_ptr + 22 + 1];

            bool ch3a = nat_file[current_ptr + 26580 + 1] & 1;

            number_of_lines++;
            image_width = number_of_samples;

            for (int c = 0; c < 6; c++)
                avhrr_data[c].resize(number_of_lines * number_of_samples);

            for (int c = 0; c < 5; c++)
            {
                int c2 = 0;
                if (c == 0)
                    c2 = 0;
                if (c == 1)
                    c2 = 1;
                if (c == 2)
                    c2 = ch3a ? 2 : 3;
                if (c == 3)
                    c2 = 4;
                if (c == 4)
                    c2 = 5;
                for (int i = 0; i < number_of_samples; i++)
                {
                    int16_t val;
                    ((uint8_t *)&val)[1] = nat_file[current_ptr + 24 + (c * number_of_samples + i) * 2 + 0];
                    ((uint8_t *)&val)[0] = nat_file[current_ptr + 24 + (c * number_of_samples + i) * 2 + 1];
                    avhrr_data[c2][(number_of_lines - 1) * image_width + i] = val + 32768;
                }
            }

            if ((number_of_lines - 1) % 50 == 0)
            {
                double lat_first = get_deg_latlon(&nat_file[current_ptr + 20538 + 0]);
                double lon_first = get_deg_latlon(&nat_file[current_ptr + 20538 + 4]);

                double lat_last = get_deg_latlon(&nat_file[current_ptr + 20546 + 0]);
                double lon_last = get_deg_latlon(&nat_file[current_ptr + 20546 + 4]);

                gcps_all[all_gcps]["x"] = 0;
                gcps_all[all_gcps]["y"] = number_of_lines - 1;
                gcps_all[all_gcps]["lat"] = lat_first;
                gcps_all[all_gcps]["lon"] = lon_first;
                all_gcps++;

                gcps_all[all_gcps]["x"] = image_width - 1;
                gcps_all[all_gcps]["y"] = number_of_lines - 1;
                gcps_all[all_gcps]["lat"] = lat_last;
                gcps_all[all_gcps]["lon"] = lon_last;
                all_gcps++;

                for (int i = 0; i < 103; i += 2)
                {
                    int px = 4 + i * 20;
                    if (px >= 2048)
                        continue;
                    double lat = get_deg_latlon(&nat_file[current_ptr + 21380 + i * 8 + 0]);
                    double lon = get_deg_latlon(&nat_file[current_ptr + 21380 + i * 8 + 4]);
                    gcps_all[all_gcps]["x"] = px;
                    gcps_all[all_gcps]["y"] = number_of_lines - 1;
                    gcps_all[all_gcps]["lat"] = lat;
                    gcps_all[all_gcps]["lon"] = lon;
                    all_gcps++;

                    // logger->trace("%f %f", lon, lat);
                }

                logger->info("%d %d %d %f %f %f %f %d", (int)main_header.record_class, (int)main_header.record_size, (int)main_header.record_subclass_version,
                             lon_first, lat_first, lon_last, lat_last, ch3a);
            }
        }

        current_ptr += main_header.record_size;
    }

    {
        satdump::ImageProducts avhrr_products;
        avhrr_products.instrument_name = "avhrr_3";
        // avhrr_products.has_timestamps = true;
        // avhrr_products.set_tle(satellite_tle);
        avhrr_products.bit_depth = 10;
        // avhrr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
        // avhrr_products.set_timestamps(avhrr_reader.timestamps);
        // avhrr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/metop_abc_avhrr.json")));

        nlohmann::json proj_cfg;
        proj_cfg["type"] = "normal_gcps";
        proj_cfg["gcp_cnt"] = all_gcps;
        proj_cfg["gcps"] = gcps_all;
        avhrr_products.set_proj_cfg(proj_cfg);

        std::string names[6] = {"1", "2", "3a", "3b", "4", "5"};
        for (int i = 0; i < 6; i++)
            avhrr_products.images.push_back({"AVHRR-" + names[i], names[i], image::Image<uint16_t>(avhrr_data[i].data(), image_width, number_of_lines, 1)});

        // calibration
        nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/AVHRR.json"));

        // calib
        nlohmann::json calib_cfg;
        calib_cfg["calibrator"] = "metop_avhrr_nat";
        avhrr_products.set_calibration(calib_cfg);
        for (int n = 0; n < 3; n++)
        {
            avhrr_products.set_calibration_type(n, avhrr_products.CALIB_REFLECTANCE);
            avhrr_products.set_calibration_type(n + 3, avhrr_products.CALIB_RADIANCE);
        }
        for (int c = 0; c < 6; c++)
            avhrr_products.set_calibration_default_radiance_range(c, calib_coefs["all"]["default_display_range"][c][0].get<double>(), calib_coefs["all"]["default_display_range"][c][1].get<double>());

        if (!std::filesystem::exists(pro_output_file))
            std::filesystem::create_directories(pro_output_file);
        avhrr_products.save(pro_output_file);
    }
}
