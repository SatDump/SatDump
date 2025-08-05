#include "ami_nc.h"
#include "../../../old/nc2pro/hdf5_utils.h"
#include "core/resources.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "projection/standard/proj_json.h"
#include "utils/string.h"
#include <H5LTpublic.h>

extern "C"
{
    void register_MTG_FILTER();
}

namespace satdump
{
    namespace official
    {
        AMINcProcessor::ParsedGK2AAMI AMINcProcessor::parse_gk2a_ami_netcdf(std::vector<uint8_t> &data)
        {
            ParsedGK2AAMI image_out;
            hsize_t image_dims[2];

            hid_t file = H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY);
            if (file < 0)
                return image_out;

            std::vector<std::string> name_parts = splitString(nc2pro::hdf5_get_string_attr_FILE_fixed(file, "file_name"), '_');
            image_out.channel = name_parts[3];
            std::transform(image_out.channel.begin(), image_out.channel.end(), image_out.channel.begin(), ::toupper);

            image_out.longitude = nc2pro::hdf5_get_double_attr_FILE(file, "sub_longitude") * 180 / M_PI;
            image_out.cfac = nc2pro::hdf5_get_double_attr_FILE(file, "cfac");
            image_out.lfac = nc2pro::hdf5_get_double_attr_FILE(file, "lfac");
            image_out.coff = nc2pro::hdf5_get_double_attr_FILE(file, "coff");
            image_out.loff = nc2pro::hdf5_get_double_attr_FILE(file, "loff");
            image_out.nominal_satellite_height = nc2pro::hdf5_get_double_attr_FILE(file, "nominal_satellite_height");
            image_out.calibration_scale = nc2pro::hdf5_get_double_attr_FILE(file, "DN_to_Radiance_Gain");
            image_out.calibration_offset = nc2pro::hdf5_get_double_attr_FILE(file, "DN_to_Radiance_Offset");
            image_out.kappa = nc2pro::hdf5_get_double_attr_FILE(file, "Radiance_to_Albedo_c");
            image_out.timestamp = nc2pro::hdf5_get_double_attr_FILE(file, "observation_start_time") + 946728000; // Jan 1, 2000 12:00 UTC
            image_out.sat_name = nc2pro::hdf5_get_string_attr_FILE_fixed(file, "satellite_name");

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
                H5Dread(dataset, H5T_NATIVE_UINT16, memspace, dataspace, H5P_DEFAULT, (uint16_t *)image_out.img.raw_data());
                int bit_depth = nc2pro::hdf5_get_float_attr(file, "image_pixel_values", "number_of_valid_bits_per_pixel");
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

        void AMINcProcessor::ingestFile(std::vector<uint8_t> vec) { all_images.push_back(parse_gk2a_ami_netcdf(vec)); }

        std::shared_ptr<satdump::products::Product> AMINcProcessor::getProduct()
        {
            // Saving
            std::shared_ptr<satdump::products::ImageProduct> ami_products = std::make_shared<satdump::products::ImageProduct>();
            ami_products->instrument_name = "ami";

            time_t prod_timestamp = 0;
            std::string sat_name = "GK-2A";
            size_t largest_width = 0, largest_height = 0;
            double center_longitude = 0;
            ParsedGK2AAMI largestImg;

            for (int i = 0; i < (int)all_images.size(); i++)
            {
                auto &img = all_images[i];
                ami_products->images.push_back({i, "AMI-" + img.channel, img.channel, img.img, 16});

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

            ami_products->set_product_timestamp(prod_timestamp);
            ami_products->set_product_source(sat_name);

            {
                nlohmann::json proj_cfg;

                constexpr double k = 624597.0334223134;
                double scalar_x = (pow(2.0, 16.0) / double(largestImg.cfac)) * k;
                double scalar_y = (pow(2.0, 16.0) / double(largestImg.lfac)) * k;

                for (auto &img : ami_products->images)
                    img.ch_transform = satdump::ChannelTransform().init_affine(double(largest_width) / img.image.width(), double(largest_height) / img.image.height(), 0, 0);

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
                ami_products->set_proj_cfg(proj_cfg);
            }

            nlohmann::json calib_cfg;
            for (size_t i = 0; i < all_images.size(); i++)
            {
                calib_cfg["vars"]["scale"][i] = all_images[i].calibration_scale;
                calib_cfg["vars"]["offset"][i] = all_images[i].calibration_offset;
                calib_cfg["vars"]["kappa"][i] = all_images[i].kappa;
            }
            ami_products->set_calibration("goes_nc_abi", calib_cfg);

            for (int i = 0; i < (int)all_images.size(); i++)
            {
                double wavelength_nm = std::stod(all_images[i].channel.substr(2, all_images[i].channel.size() - 1)) * 100;
                ami_products->set_channel_wavenumber(i, 1e7 / wavelength_nm);
                if (all_images[i].kappa > 0)
                    ami_products->set_channel_unit(i, CALIBRATION_ID_ALBEDO);
                else
                    ami_products->set_channel_unit(i, CALIBRATION_ID_EMISSIVE_RADIANCE);
            }

            return ami_products;
        }
    } // namespace official
} // namespace satdump