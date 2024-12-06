#define _USE_MATH_DEFINES
#include "mtg_fci.h"

#include "libs/rapidxml.hpp"
#include "common/utils.h"

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

#include "core/exception.h"

extern "C"
{
    void register_MTG_FILTER();
}

namespace nc2pro
{
    struct ParsedGOESRABI
    {
        image::Image img;
        int channel;
        double longitude;
        double x_add_offset, y_add_offset;
        double x_scale_factor, y_scale_factor;
        double perspective_point_height;
        double calibration_scale;
        double calibration_offset;
        time_t timestamp;
        std::string goes_sat;
        std::string time_coverage_start;
    };

    int hdf5_get_int(hid_t &file, std::string path);
    float hdf5_get_float_attr(hid_t &file, std::string path, std::string attr);
    std::string hdf5_get_string_attr_FILE(hid_t &file, std::string attr);
    std::string hdf5_get_string_attr_FILE_fixed(hid_t &file, std::string attr);

    ParsedGOESRABI parse_goes_abi_netcdf_fulldisk(std::vector<uint8_t> data)
    {
        ParsedGOESRABI image_out;

        // herr_t status;
        hsize_t image_dims[2];

        hid_t file = H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY);

        if (file < 0)
            return image_out;

        //    bool is_hr_mode = true;

        //    {
        //       hid_t dataset = H5Dopen1(file, "l1c_channels_present");
        //       if (dataset < 0)
        //           return image_out;
        //       hsize_t l1c_channels[2];
        //       hid_t dataspace = H5Dget_space(dataset); /* dataspace handle */
        //       H5Sget_simple_extent_dims(dataspace, l1c_channels, NULL);
        //       is_hr_mode = l1c_channels[0] == 4;
        //      H5Dclose(dataset);
        //  }

        //    image_out.platform_name = "GOES-16"; // hdf5_get_string_attr_FILE(file, "platform");
        //    image_out.time_coverage_start = "0"; // hdf5_get_string_attr_FILE(file, "time_coverage_start");
        //    image_out.longitude = 0;             // hdf5_get_float_attr(file, "data/mtg_geos_projection", "longitude_of_projection_origin");

        {
            //  auto test_path = (std::string) "Rad";//"data/" + std::string(is_hr_mode ? channel_map_hr[ch] : channel_map[ch]);
            //  auto channel_path = (std::string) "data/" + std::string(is_hr_mode ? channel_map_hr[ch] : channel_map[ch]) + "/measured/effective_radiance";
            //  auto rowoff_path = (std::string) "data/" + std::string(is_hr_mode ? channel_map_hr[ch] : channel_map[ch]) + "/measured/start_position_row";

            image_out.channel = hdf5_get_int(file, "band_id");
            image_out.x_add_offset = hdf5_get_float_attr(file, "x", "add_offset");
            image_out.y_add_offset = hdf5_get_float_attr(file, "y", "add_offset");
            image_out.x_scale_factor = hdf5_get_float_attr(file, "x", "scale_factor");
            image_out.y_scale_factor = hdf5_get_float_attr(file, "y", "scale_factor");
            image_out.perspective_point_height = hdf5_get_float_attr(file, "goes_imager_projection", "perspective_point_height");
            image_out.longitude = hdf5_get_float_attr(file, "goes_imager_projection", "longitude_of_projection_origin");
            image_out.goes_sat = hdf5_get_string_attr_FILE_fixed(file, "platform_ID");
            image_out.time_coverage_start = hdf5_get_string_attr_FILE_fixed(file, "time_coverage_start");
            image_out.calibration_scale = hdf5_get_float_attr(file, "x", "scale_factor");
            image_out.calibration_offset = hdf5_get_float_attr(file, "y", "add_offset");

            if (!H5Lexists(file, "Rad", H5P_DEFAULT))
                goto close;

            //     image_out.calibration_scale[ch] = hdf5_get_float_attr(file, channel_path, "scale_factor");
            //     image_out.calibration_offset[ch] = hdf5_get_float_attr(file, channel_path, "add_offset");

            //      {
            //          int start_row = hdf5_get_int(file, rowoff_path);
            //          if (start_row == -1e6)
            //              continue;
            //          image_out.start_row[ch] = start_row;
            //      }

            hid_t dataset = H5Dopen2(file, "Rad", H5P_DEFAULT);

            if (dataset < 0)
                goto close;

            hid_t dataspace = H5Dget_space(dataset);
            int rank = H5Sget_simple_extent_ndims(dataspace);
            H5Sget_simple_extent_dims(dataspace, image_dims, NULL);

            if (rank != 2)
                return image_out;

            hid_t memspace = H5Screate_simple(2, image_dims, NULL);

            image::Image img(16, image_dims[1], image_dims[0], 1);

            H5Dread(dataset, H5T_NATIVE_UINT16, memspace, dataspace, H5P_DEFAULT, (uint16_t *)img.raw_data());

            for (size_t i = 0; i < img.size(); i++)
                if (img.get(i) == (image_out.channel == 2 ? 4095 : 1023))
                    img.set(i, 0);
                else
                    img.set(i, img.get(i) << (image_out.channel == 2 ? 4 : 6));

            image_out.img = img;

            H5Dclose(dataset);
        }
    close:

        H5Fclose(file);

        return image_out;
    }

    void process_goesr_abi(std::string nc_file, std::string pro_output_file, double *progess)
    {
        //          double calibration_scale[16] = {0};
        //     double calibration_offset[16] = {0};

        float center_longitude = 0;
        std::vector<ParsedGOESRABI> all_images;

        time_t prod_timestamp = 0;
        std::string satellite = "Unknown GOES-R";

        /*{
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
                            auto img = parse_mtg_fci_netcdf_fulldisk(vec);
                            all_images.push_back(img);

                            if (img.platform_name == "MTI1")
                                satellite = "MTG-I1";
                            else if (img.platform_name == "MTI2")
                                satellite = "MTG-I2";

                            std::tm timeS;
                            memset(&timeS, 0, sizeof(std::tm));
                            if (sscanf(img.time_coverage_start.c_str(),
                                       "%4d%2d%2d%2d%2d%2d",
                                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
                            {
                                timeS.tm_year -= 1900;
                                timeS.tm_mon -= 1;
                                timeS.tm_isdst = -1;
                                time_t ctime = timegm(&timeS);
                                if (ctime < prod_timestamp || prod_timestamp == 0)
                                    prod_timestamp = ctime;
                            }

                            for (int i = 0; i < 16; i++)
                            {
                                calibration_scale[i] = img.calibration_scale[i];
                                calibration_offset[i] = img.calibration_offset[i];
                            }

                            mz_free(file_ptr);
                        }
                    }
                }

                *progess = double(fi) / double(numfiles);
            }

            mz_zip_reader_end(&zip);
        }*/

        std::vector<std::string> files_to_parse; //= {nc_file};
        {
            auto ori_splt = splitString(std::filesystem::path(nc_file).stem().string(), '_');
            if (ori_splt.size() != 6)
                throw satdump_exception("Invalid GOES-R filename!");
            auto ori_splt2 = splitString(ori_splt[1], '-');
            if (ori_splt2.size() != 4 || ori_splt2[3].size() != 5)
                throw satdump_exception("Invalid GOES-R filename!");

            std::filesystem::recursive_directory_iterator pluginIterator(std::filesystem::path(nc_file).parent_path());
            std::error_code iteratorError;
            while (pluginIterator != std::filesystem::recursive_directory_iterator())
            {
                std::string path = pluginIterator->path().string();
                if (!std::filesystem::is_regular_file(pluginIterator->path()) || pluginIterator->path().extension() != ".nc")
                    goto skip_this;

                {
                    std::string cleanname = std::filesystem::path(path).stem().string();
                    auto splt = splitString(cleanname, '_');

                    if (splt.size() == 6 &&
                        ori_splt[0] == splt[0] &&
                        ori_splt[2] == splt[2] &&
                        ori_splt[3] == splt[3])
                    {
                        auto splt2 = splitString(splt[1], '-');

                        if (splt2.size() == 4 &&
                            ori_splt2[0] == splt2[0] &&
                            ori_splt2[1] == splt2[1] &&
                            ori_splt2[2] == splt2[2] &&
                            ori_splt2[3].size() == 5 &&
                            ori_splt2[3][0] == splt2[3][0] &&
                            ori_splt2[3][1] == splt2[3][1] &&
                            ori_splt2[3][2] == splt2[3][2])
                        {
                            files_to_parse.push_back(path);
                            //                            logger->trace("Selecting " + path);
                        }
                    }
                }

            skip_this:
                pluginIterator.increment(iteratorError);
                if (iteratorError)
                    logger->critical(iteratorError.message());
            }
        }

        for (auto &f : files_to_parse)
        {
            std::vector<uint8_t> nat_file;
            {
                std::ifstream input_file(f, std::ios::binary);
                uint8_t byte;
                while (!input_file.eof())
                {
                    input_file.read((char *)&byte, 1);
                    nat_file.push_back(byte);
                    // progress++;
                }
            }

            logger->info("Processing " + f);
            all_images.push_back(parse_goes_abi_netcdf_fulldisk(nat_file));
        }

        if (prod_timestamp == 0)
            prod_timestamp = time(0);

        if (all_images.size() > 0)
        {
            std::tm timeS;
            memset(&timeS, 0, sizeof(std::tm));
            if (sscanf(all_images[0].time_coverage_start.c_str(),
                       "%4d-%2d-%2dT%2d:%2d:%2d.%*dZ",
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                timeS.tm_isdst = -1;
                time_t ctime = timegm(&timeS);
                if (ctime < prod_timestamp || prod_timestamp == 0)
                    prod_timestamp = ctime;
            }
        }

        // Saving
        satdump::ImageProducts abi_products;
        abi_products.instrument_name = "abi";
        abi_products.has_timestamps = false;
        abi_products.bit_depth = 10;

        double calibration_scale[16];
        double calibration_offset[16];

        int largest_width = 0, largest_height = 0;
        ParsedGOESRABI largestImg;
        for (auto &img : all_images)
        {
            abi_products.images.push_back({"ABI-" + std::to_string(img.channel), std::to_string(img.channel), img.img});

            bool waslarger = false;
            if (img.img.width() > largest_width)
            {
                largest_width = img.img.width();
                waslarger = true;
            }
            if (img.img.height() > largest_height)
            {
                largest_height = img.img.height();
                waslarger = true;
            }
            img.img.clear();
            if (waslarger)
                largestImg = img;

            if (img.goes_sat == "G16")
                satellite = "GOES-16";
            else if (img.goes_sat == "G17")
                satellite = "GOES-17";
            else if (img.goes_sat == "G18")
                satellite = "GOES-18";
            else if (img.goes_sat == "G19")
                satellite = "GOES-19";

            calibration_scale[img.channel - 1] = img.calibration_scale;
            calibration_offset[img.channel - 1] = img.calibration_offset;
            center_longitude = img.longitude;
        }

        abi_products.set_product_timestamp(prod_timestamp);
        abi_products.set_product_source(satellite);

        {
            nlohmann::json proj_cfg;

            double line_scalar = std::abs(largestImg.perspective_point_height * largestImg.y_scale_factor);
            double line_offset = -largestImg.y_add_offset / largestImg.y_scale_factor;
            double column_scalar = std::abs(largestImg.perspective_point_height * largestImg.x_scale_factor);
            double column_offset = -largestImg.x_add_offset / largestImg.x_scale_factor;

            constexpr double k = 624597.0334223134;
            double scalar_x = column_scalar;
            double scalar_y = line_scalar;

            proj_cfg["type"] = "geos";
            proj_cfg["lon0"] = center_longitude;
            proj_cfg["sweep_x"] = true;
            proj_cfg["scalar_x"] = scalar_x;
            proj_cfg["scalar_y"] = -scalar_y;
            proj_cfg["offset_x"] = double(column_offset) * -scalar_x;
            proj_cfg["offset_y"] = double(line_offset) * scalar_y;
            proj_cfg["width"] = largest_width;
            proj_cfg["height"] = largest_height;
            proj_cfg["altitude"] = 35786023.00;
            abi_products.set_proj_cfg(proj_cfg);
        }

#if 0
        const float goes_abi_wavelength_table[16] = {
            470,
            640,
            860,
            1380,
            1610,
            2260,
            3900,
            6190,
            6950,
            7340,
            8500,
            9610,
            10350,
            11200,
            12300,
            13300,
        };
        const float goes_abi_radiance_ranges_table[16][2] = {
            {0, 1},
            {0, 1},
            {0, 1},
            {0, 1},
            {0, 1},
            {0, 1},
            {0.01, 2.20}, // 3900,
            {0, 6},       // 6190,
            {0, 16},      // 6950,
            {0.02, 20},   // 7340,
            {5, 100},     // 8500,
            {10, 120},    // 9610,
            {10, 120},    // 10350,
            {10, 150},    // 11200,
            {20, 150},    // 12300,
            {20, 150},    // 13300,
        };

        nlohmann::json calib_cfg;
        calib_cfg["calibrator"] = "mtg_nc_fci";
        for (int i = 0; i < 16; i++)
        {
            calib_cfg["vars"]["scale"][i] = calibration_scale[i];
            calib_cfg["vars"]["offset"][i] = calibration_offset[i];
        }
        abi_products.set_calibration(calib_cfg);

        int ii2 = 0;
        for (int i = 0; i < all_images.size(); i++)
        {
            abi_products.set_calibration_type(ii2, satdump::ImageProducts::CALIB_RADIANCE);
            abi_products.set_wavenumber(ii2, 1e7 / goes_abi_wavelength_table[all_images[i].channel - 1]);
            abi_products.set_calibration_default_radiance_range(ii2, goes_abi_radiance_ranges_table[all_images[i].channel - 1][0], goes_abi_radiance_ranges_table[all_images[i].channel - 1][1]);
        }
#endif

#if 0
        nlohmann::json fci_config = loadJsonFile(resources::getResourcePath("calibration/FCI_table.json"));
#endif

        if (!std::filesystem::exists(pro_output_file))
            std::filesystem::create_directories(pro_output_file);
        abi_products.save(pro_output_file);
    }
}
