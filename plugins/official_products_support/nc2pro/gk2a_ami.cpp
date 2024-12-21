#define _USE_MATH_DEFINES
#include <cmath>
#include <filesystem>
#include <algorithm>

#include "gk2a_ami.h"
#include "hdf5_utils.h"
#include <H5LTpublic.h>
#include "common/image/image.h"
#include "common/utils.h"
#include "core/exception.h"
#include "products/image_products.h"
#include "logger.h"

namespace nc2pro
{
    struct ParsedGK2AAMI
    {
        image::Image img;
        std::string channel;
        double longitude;
        double cfac, lfac;
        double coff, loff;
        double nominal_satellite_height;
        double calibration_scale;
        double calibration_offset;
        double kappa;
        time_t timestamp;
        std::string sat_name;
    };

    ParsedGK2AAMI parse_gk2a_ami_netcdf(std::vector<uint8_t> &data)
    {
        ParsedGK2AAMI image_out;
        hsize_t image_dims[2];

        hid_t file = H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY);
        if (file < 0)
            return image_out;

        std::vector<std::string> name_parts = splitString(hdf5_get_string_attr_FILE_fixed(file, "file_name"), '_');
        image_out.channel = name_parts[3];
        std::transform(image_out.channel.begin(), image_out.channel.end(), image_out.channel.begin(), ::toupper);

        image_out.longitude = hdf5_get_double_attr_FILE(file, "sub_longitude") * 180 / M_PI;
        image_out.cfac = hdf5_get_double_attr_FILE(file, "cfac");
        image_out.lfac = hdf5_get_double_attr_FILE(file, "lfac");
        image_out.coff = hdf5_get_double_attr_FILE(file, "coff");
        image_out.loff = hdf5_get_double_attr_FILE(file, "loff");
        image_out.nominal_satellite_height = hdf5_get_double_attr_FILE(file, "nominal_satellite_height");
        image_out.calibration_scale = hdf5_get_double_attr_FILE(file, "DN_to_Radiance_Gain");
        image_out.calibration_offset = hdf5_get_double_attr_FILE(file, "DN_to_Radiance_Offset");
        image_out.kappa = hdf5_get_double_attr_FILE(file, "Radiance_to_Albedo_c");
        image_out.timestamp = hdf5_get_double_attr_FILE(file, "observation_start_time") + 946728000; // Jan 1, 2000 12:00 UTC
        image_out.sat_name = hdf5_get_string_attr_FILE_fixed(file, "satellite_name");

        {
            if (!H5Lexists(file, "image_pixel_values", H5P_DEFAULT))
                goto close;

            hid_t dataset = H5Dopen2(file, "image_pixel_values", H5P_DEFAULT);
            if (dataset < 0)
                goto close;

            hid_t dataspace = H5Dget_space(dataset);
            int rank = H5Sget_simple_extent_ndims(dataspace);
            H5Sget_simple_extent_dims(dataspace, image_dims, NULL);

            if (rank != 2)
                goto close;

            hid_t memspace = H5Screate_simple(2, image_dims, NULL);
            image_out.img = image::Image(16, image_dims[1], image_dims[0], 1);
            H5Dread(dataset, H5T_NATIVE_UINT16, memspace, dataspace, H5P_DEFAULT, (uint16_t*)image_out.img.raw_data());
            int bit_depth = hdf5_get_float_attr(file, "image_pixel_values", "number_of_valid_bits_per_pixel");
            int bit_depth_exponentiation = pow(2, bit_depth) - 1;
            image_out.calibration_scale /= pow(2, 16 - bit_depth);

            for (size_t i = 0; i < image_out.img.size(); i++)
            {
                if (image_out.img.get(i) == bit_depth_exponentiation)
                    image_out.img.set(i, 0);
                else
                    image_out.img.set(i, image_out.img.get(i) << (16 - bit_depth));
            }

            H5Dclose(dataset);
        }

    close:
        H5Fclose(file);
        return image_out;
    }

    void process_gk2a_ami(std::string nc_file, std::string pro_output_file, double *progress)
    {
        std::vector<ParsedGK2AAMI> all_images;
        std::vector<std::string> files_to_parse; //= {nc_file};
        {
            auto ori_splt = splitString(std::filesystem::path(nc_file).stem().string(), '_');
            if (ori_splt.size() != 6)
                throw satdump_exception("Invalid GK-2A filename!");

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
                        ori_splt[1] == splt[1] &&
                        ori_splt[2] == splt[2] && 
                        ori_splt[5] == splt[5])
                    {
                        files_to_parse.push_back(path);
                    }
                }

            skip_this:
                pluginIterator.increment(iteratorError);
                if (iteratorError)
                    logger->critical(iteratorError.message());
            }
        }

        for (size_t i = 0; i < files_to_parse.size(); i++)
        {
            std::vector<uint8_t> nc_file;
            std::ifstream input_file(files_to_parse[i], std::ios::binary);
            input_file.seekg(0, std::ios::end);
            const size_t nc_size = input_file.tellg();
            nc_file.resize(nc_size);
            input_file.seekg(0, std::ios::beg);
            input_file.read((char*)&nc_file[0], nc_size);
            input_file.close();

            logger->info("Processing " + files_to_parse[i]);
            all_images.push_back(parse_gk2a_ami_netcdf(nc_file));
            *progress = double(i + 1) / double(files_to_parse.size());
        }

        // Saving
        satdump::ImageProducts ami_products;
        ami_products.instrument_name = "ami";
        ami_products.has_timestamps = false;
        ami_products.bit_depth = 16;

        time_t prod_timestamp = 0;
        std::string sat_name = "GK-2A";
        size_t largest_width = 0, largest_height = 0;
        double center_longitude = 0;
        ParsedGK2AAMI largestImg;

        for (auto &img : all_images)
        {
            ami_products.images.push_back({ "AMI-" + img.channel, img.channel, img.img });

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

            prod_timestamp = img.timestamp;
            sat_name = img.sat_name;
            center_longitude = img.longitude;
        }

        ami_products.set_product_timestamp(prod_timestamp);
        ami_products.set_product_source(sat_name);

        {
            nlohmann::json proj_cfg;

            constexpr double k = 624597.0334223134;
            double scalar_x = (pow(2.0, 16.0) / double(largestImg.cfac)) * k;
            double scalar_y = (pow(2.0, 16.0) / double(largestImg.lfac)) * k;

            proj_cfg["type"] = "geos";
            proj_cfg["lon0"] = center_longitude;
            proj_cfg["sweep_x"] = false;
            proj_cfg["scalar_x"] = scalar_x;
            proj_cfg["scalar_y"] = scalar_y;
            proj_cfg["offset_x"] = double(largestImg.coff) * -scalar_x;
            proj_cfg["offset_y"] = double(largestImg.loff) * -scalar_y;
            proj_cfg["width"] = largest_width;
            proj_cfg["height"] = largest_height;
            proj_cfg["altitude"] = 35786023.00;
            ami_products.set_proj_cfg(proj_cfg);
        }

        nlohmann::json calib_cfg;
        calib_cfg["calibrator"] = "goes_nc_abi";
        for (size_t i = 0; i < all_images.size(); i++)
        {
            calib_cfg["vars"]["scale"][i] = all_images[i].calibration_scale;
            calib_cfg["vars"]["offset"][i] = all_images[i].calibration_offset;
            calib_cfg["vars"]["kappa"][i] = all_images[i].kappa;
        }
        ami_products.set_calibration(calib_cfg);

        std::map<std::string, std::pair<float, float>> ami_ranges = {
            {"VI004", {0, 1}},
            {"VI005", {0, 1}},
            {"VI006", {0, 1}},
            {"VI008", {0, 1}},
            {"NR013", {0, 1}},
            {"NR016", {0, 1}},
            {"SW038", {0.01, 2.20}},
            {"WV063", {0, 6}},
            {"WV069", {0, 16}},
            {"WV073", {0.02, 20}},
            {"IR087", {5, 100}},
            {"IR096", {10, 120}},
            {"IR105", {10, 120}},
            {"IR112", {10, 150}},
            {"IR123", {20, 150}},
            {"IR133", {20, 150}},
        };

        for (int i = 0; i < (int)all_images.size(); i++)
        {
            double wavelength_nm = std::stod(all_images[i].channel.substr(2, all_images[i].channel.size() - 1)) * 100;
            ami_products.set_wavenumber(i, 1e7 / wavelength_nm);
            ami_products.set_calibration_default_radiance_range(i, ami_ranges[all_images[i].channel].first, ami_ranges[all_images[i].channel].second);
            if (all_images[i].kappa > 0)
                ami_products.set_calibration_type(i, satdump::ImageProducts::CALIB_REFLECTANCE);
            else
                ami_products.set_calibration_type(i, satdump::ImageProducts::CALIB_RADIANCE);
        }

        if (!std::filesystem::exists(pro_output_file))
            std::filesystem::create_directories(pro_output_file);
        ami_products.save(pro_output_file);
    }
}