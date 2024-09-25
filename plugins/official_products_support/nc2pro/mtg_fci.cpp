#define _USE_MATH_DEFINES
#include "mtg_fci.h"

#include <hdf5.h>
#include <H5LTpublic.h>
#include "common/image/image.h"
#include <filesystem>
#include "logger.h"
#include <array>
#include "products/image_products.h"
#include "libs/miniz/miniz.h"

#include "common/projection/projs2/proj_json.h"
#include "common/calibration.h"

#include "nlohmann/json_utils.h"
#include "resources.h"

#include "libs/rapidxml.hpp"

extern "C"
{
    void register_MTG_FILTER();
}

namespace nc2pro
{
    int hdf5_get_int(hid_t &file, std::string path)
    {
        int32_t val = 0;
        hid_t dataset = H5Dopen1(file, path.c_str());
        if (dataset < 0)
            return -1e6;
        H5Dread(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, &val);
        H5Dclose(dataset);
        return val;
    }

    float hdf5_get_float_attr(hid_t &file, std::string path, std::string attr)
    {
        float val = -1e6;
        hid_t dataset = H5Dopen1(file, path.c_str());
        if (dataset < 0)
            return -1e6;
        hid_t att = H5Aopen(dataset, attr.c_str(), H5P_DEFAULT);
        H5Aread(att, H5T_NATIVE_FLOAT, &val);
        H5Aclose(att);
        H5Dclose(dataset);
        return val;
    }

    std::array<std::pair<int, image::Image>, 16> parse_mtg_fci_netcdf_fulldisk(std::vector<uint8_t> data, int bit_depth, float &longitude,
                                                                               double *calibration_scale, double *calibration_offset)
    {
        std::array<std::pair<int, image::Image>, 16> image_out;

        // herr_t status;
        hsize_t image_dims[2];

        hid_t file = H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY);

        if (file < 0)
            return image_out;

        bool is_hr_mode = true;

        {
            hid_t dataset = H5Dopen1(file, "l1c_channels_present");
            if (dataset < 0)
                return image_out;
            hsize_t l1c_channels[2];
            hid_t dataspace = H5Dget_space(dataset); /* dataspace handle */
            H5Sget_simple_extent_dims(dataspace, l1c_channels, NULL);
            is_hr_mode = l1c_channels[0] == 4;
            H5Dclose(dataset);
        }

        {
            float lon = hdf5_get_float_attr(file, "data/mtg_geos_projection", "longitude_of_projection_origin");
            longitude = lon;
        }

        const std::string channel_map[16] = {
            "vis_04",
            "vis_05",
            "vis_06",
            "vis_08",
            "vis_09",
            "nir_13",
            "nir_16",
            "nir_22",
            "ir_38",
            "wv_63",
            "wv_73",
            "ir_87",
            "ir_97",
            "ir_105",
            "ir_123",
            "ir_133",
        };

        const std::string channel_map_hr[16] = {
            "_",
            "_",
            "vis_06_hr",
            "_",
            "_",
            "_",
            "_",
            "nir_22_hr",
            "ir_38_hr",
            "_",
            "_",
            "_",
            "_",
            "ir_105_hr",
            "_",
            "_",
        };

        for (int ch = 0; ch < 16; ch++)
        {
            auto test_path = (std::string) "data/" + std::string(is_hr_mode ? channel_map_hr[ch] : channel_map[ch]);
            auto channel_path = (std::string) "data/" + std::string(is_hr_mode ? channel_map_hr[ch] : channel_map[ch]) + "/measured/effective_radiance";
            auto rowoff_path = (std::string) "data/" + std::string(is_hr_mode ? channel_map_hr[ch] : channel_map[ch]) + "/measured/start_position_row";

            if (!H5Lexists(file, test_path.c_str(), H5P_DEFAULT))
            {
                logger->trace("Skipping channel %d", ch + 1);
                continue;
            }

            calibration_scale[ch] = hdf5_get_float_attr(file, channel_path, "scale_factor");
            calibration_offset[ch] = hdf5_get_float_attr(file, channel_path, "add_offset");

            {
                int start_row = hdf5_get_int(file, rowoff_path);
                if (start_row == -1e6)
                    continue;
                image_out[ch].first = start_row;
            }

            hid_t dataset = H5Dopen2(file, channel_path.c_str(), H5P_DEFAULT);

            if (dataset < 0)
                continue;

            hid_t dataspace = H5Dget_space(dataset); /* dataspace handle */
            int rank = H5Sget_simple_extent_ndims(dataspace);
            /*int status_n =*/H5Sget_simple_extent_dims(dataspace, image_dims, NULL);

            if (rank != 2)
                return image_out;

            hid_t memspace = H5Screate_simple(2, image_dims, NULL);

            image::Image img(16, image_dims[1], image_dims[0], 1);

            /*status =*/H5Dread(dataset, H5T_NATIVE_UINT16, memspace, dataspace, H5P_DEFAULT, (uint16_t *)img.raw_data());

            for (size_t i = 0; i < img.size(); i++)
            {
                if (img.get(i) == 65535)
                    img.set(i, 0);
                else
                    img.set(i, img.get(i) << (16 - bit_depth));
            }

            image_out[ch].second = img;

            H5Dclose(dataset);
        }

        H5Fclose(file);

        return image_out;
    }

    void process_mtg_fci(std::string zip_file, std::string pro_output_file, double *progess)
    {
        register_MTG_FILTER();

        double calibration_scale[16] = {0};
        double calibration_offset[16] = {0};

        float center_longitude = 0;
        std::vector<std::array<std::pair<int, image::Image>, 16>> all_images;

        time_t prod_timestamp = time(0);
        std::string satellite = "Unknown MTG-I";

        {
            mz_zip_archive zip{};
            mz_zip_reader_init_file(&zip, zip_file.c_str(), 0);

            //        logger->info("%s", mz_zip_get_error_string(mz_zip_get_last_error(&zip)));

            int numfiles = mz_zip_reader_get_num_files(&zip);

            for (int fi = 0; fi < numfiles; fi++)
            {
                if (mz_zip_reader_is_file_supported(&zip, fi))
                {
                    char filename[1000];
                    mz_zip_reader_get_filename(&zip, fi, filename, 1000);
                    std::string name = std::filesystem::path(filename).stem().string();
                    std::string ext = std::filesystem::path(filename).extension().string();
                    if (ext == ".nc")
                    {
                        logger->info("Chunk : %s", filename);

                        if (name.find("CHK-BODY") != std::string::npos)
                        {
                            size_t filesize = 0;
                            void *file_ptr = mz_zip_reader_extract_to_heap(&zip, fi, &filesize, 0);

                            std::vector<uint8_t> vec((uint8_t *)file_ptr, (uint8_t *)file_ptr + filesize);
                            auto img = parse_mtg_fci_netcdf_fulldisk(vec, 12, center_longitude, calibration_scale, calibration_offset);
                            all_images.push_back(img);

                            mz_free(file_ptr);
                        }
                    }
                    else if (name == "EOPMetadata" && ext == ".xml")
                    {
                        logger->info("Parsing XML!");

                        size_t filesize = 0;
                        void *file_ptr = mz_zip_reader_extract_to_heap(&zip, fi, &filesize, 0);

                        try
                        {
                            std::string xml_str((char *)file_ptr, filesize);
                            xml_str += '\0'; // For some reason...
                            rapidxml::xml_document<> doc;
                            doc.parse<0>((char *)xml_str.c_str());
                            std::string timestamp = doc.first_node("eum:EarthObservation")
                                                        ->first_node("om:phenomenonTime")
                                                        ->first_node("gml:TimePeriod")
                                                        ->first_node("gml:beginPosition")
                                                        ->value();
                            std::string sat_id = doc.first_node("eum:EarthObservation")
                                                     ->first_node("om:procedure")
                                                     ->first_node("eop:EarthObservationEquipment")
                                                     ->first_node("eop:platform")
                                                     ->first_node("eop:Platform")
                                                     ->first_node("eop:shortName")
                                                     ->value();

                            if (sat_id == "MTI1")
                                satellite = "MTG-I1";
                            else if (sat_id == "MTI2")
                                satellite = "MTG-I2";

                            std::tm timeS;
                            memset(&timeS, 0, sizeof(std::tm));
                            if (sscanf(timestamp.c_str(),
                                       "%4d-%2d-%2dT%2d:%2d:%2d.%*dZ",
                                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
                            {
                                timeS.tm_year -= 1900;
                                timeS.tm_mon -= 1;
                                timeS.tm_isdst = -1;
                                prod_timestamp = timegm(&timeS);
                            }
                        }
                        catch (std::exception &e)
                        {
                            logger->error("Error parsing XML : %s", e.what());
                        }

                        mz_free(file_ptr);
                    }
                }

                *progess = double(fi) / double(numfiles);
            }
        }

        image::Image final_imgs[16];
        bool final_imgs_present[16] = {false};

        // Saving
        satdump::ImageProducts fci_products;
        fci_products.instrument_name = "fci";
        fci_products.has_timestamps = false;
        fci_products.bit_depth = 12;
        fci_products.set_product_timestamp(prod_timestamp);
        fci_products.set_product_source(satellite);

        int largest_width = 0;
        int largest_height = 0;

        for (int i = 0; i < 16; i++)
        {
            int final_x = 0, final_y = 0;
            for (auto &img : all_images)
            {
                final_x = img[i].second.width();
                final_y += img[i].second.height();
            }

            final_imgs[i] = image::Image(16, final_x, final_y, 1);

            if (final_x > largest_width)
                largest_width = final_x;
            if (final_y > largest_height)
                largest_height = final_y;

            for (auto &img : all_images)
            {
                if (img[i].second.depth() == final_imgs[i].depth())
                {
                    final_imgs[i].draw_image(0, img[i].second, 0, img[i].first);
                    final_imgs_present[i] = true;
                }
                img[i].second.clear();
            }

            final_imgs[i].mirror(false, true);

            if (final_imgs_present[i])
                fci_products.images.push_back({"FCI-" + std::to_string(i + 1), std::to_string(i + 1), final_imgs[i]});

            logger->info("Processing channel %d", i + 1);
            final_imgs[i].clear();
        }

        proj::projection_t proj;
        proj.type = proj::ProjType_Geos;
        proj.params.sweep_x = false;
        proj.params.altitude = 3.57864e7;
        proj.proj_offset_x = -5568748.275756353;
        proj.proj_offset_y = 5568748.275756353;
        proj.proj_scalar_x = 3000.403165817 * (3712.0 / double(largest_width));
        proj.proj_scalar_y = -3000.403165817 * (3712.0 / double(largest_height));
        proj.lam0 = center_longitude * DEG2RAD;
        nlohmann::json proj_cfg = proj;
        proj_cfg["width"] = largest_width;
        proj_cfg["height"] = largest_height; // 3712;
        fci_products.set_proj_cfg(proj_cfg);

        nlohmann::json fci_config = loadJsonFile(resources::getResourcePath("calibration/FCI_table.json"));

        nlohmann::json calib_cfg;
        calib_cfg["calibrator"] = "mtg_nc_fci";
        for (int i = 0; i < 16; i++)
        {
            calib_cfg["vars"]["scale"][i] = calibration_scale[i];
            calib_cfg["vars"]["offset"][i] = calibration_offset[i];
        }
        fci_products.set_calibration(calib_cfg);

        int ii2 = 0;
        for (int i = 0; i < 16; i++)
        {
            if (final_imgs_present[i])
            {
                fci_products.set_calibration_type(ii2, satdump::ImageProducts::CALIB_RADIANCE);
                fci_products.set_wavenumber(ii2, freq_to_wavenumber(299792458.0 / (fci_config["wavelengths"][i].get<double>())));
                fci_products.set_calibration_default_radiance_range(ii2, fci_config["default_ranges"][i][0].get<double>(), fci_config["default_ranges"][i][1].get<double>());
                ii2++;
            }
        }

        if (!std::filesystem::exists(pro_output_file))
            std::filesystem::create_directories(pro_output_file);
        fci_products.save(pro_output_file);
    }
}
