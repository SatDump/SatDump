#include "abi_nc.h"
#include "../../../old/nc2pro/hdf5_utils.h"
#include "common/utils.h"
#include <H5LTpublic.h>

extern "C"
{
    void register_MTG_FILTER();
}

namespace satdump
{
    namespace firstparty
    {
        ABINcProcessor::ParsedGOESRABI ABINcProcessor::parse_goes_abi_netcdf(std::vector<uint8_t> &data)
        {
            ParsedGOESRABI image_out;

            hsize_t image_dims[2];

            hid_t file = H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY);

            if (file < 0)
                return image_out;

            {
                image_out.channel = nc2pro::hdf5_get_int(file, "band_id");
                image_out.x_add_offset = nc2pro::hdf5_get_float_attr(file, "x", "add_offset");
                image_out.y_add_offset = nc2pro::hdf5_get_float_attr(file, "y", "add_offset");
                image_out.x_scale_factor = nc2pro::hdf5_get_float_attr(file, "x", "scale_factor");
                image_out.y_scale_factor = nc2pro::hdf5_get_float_attr(file, "y", "scale_factor");
                image_out.perspective_point_height = nc2pro::hdf5_get_float_attr(file, "goes_imager_projection", "perspective_point_height");
                image_out.longitude = nc2pro::hdf5_get_float_attr(file, "goes_imager_projection", "longitude_of_projection_origin");
                image_out.goes_sat = nc2pro::hdf5_get_string_attr_FILE_fixed(file, "platform_ID");
                image_out.time_coverage_start = nc2pro::hdf5_get_string_attr_FILE_fixed(file, "time_coverage_start");
                image_out.calibration_scale = nc2pro::hdf5_get_float_attr(file, "Rad", "scale_factor");
                image_out.calibration_offset = nc2pro::hdf5_get_float_attr(file, "Rad", "add_offset");
                image_out.kappa = nc2pro::hdf5_get_float(file, "kappa0");
                int bit_depth = nc2pro::hdf5_get_float_attr(file, "Rad", "sensor_band_bit_depth");
                int bit_depth_exponentiation = pow(2, bit_depth) - 1;

                image_out.calibration_scale /= pow(2, 14 - bit_depth);

                if (!H5Lexists(file, "Rad", H5P_DEFAULT))
                    goto close;

                hid_t dataset = H5Dopen2(file, "Rad", H5P_DEFAULT);

                if (dataset < 0)
                    goto close;

                hid_t dataspace = H5Dget_space(dataset);
                int rank = H5Sget_simple_extent_ndims(dataspace);
                H5Sget_simple_extent_dims(dataspace, image_dims, NULL);

                if (rank != 2)
                    goto close;

                hid_t memspace = H5Screate_simple(2, image_dims, NULL);

                image::Image img(16, image_dims[1], image_dims[0], 1);

                H5Dread(dataset, H5T_NATIVE_UINT16, memspace, dataspace, H5P_DEFAULT, (uint16_t *)img.raw_data());

                for (size_t i = 0; i < img.size(); i++)
                    if (img.get(i) == bit_depth_exponentiation)
                        img.set(i, 0);
                    else
                        img.set(i, img.get(i) << (16 - bit_depth));

                image_out.img = img;

                H5Dclose(dataset);
            }
        close:

            H5Fclose(file);

            return image_out;
        }

        void ABINcProcessor::ingestFile(std::vector<uint8_t> vec) { all_images.push_back(parse_goes_abi_netcdf(vec)); }

        std::shared_ptr<satdump::products::Product> ABINcProcessor::getProduct()
        {
            if (prod_timestamp == 0)
                prod_timestamp = time(0);

            if (all_images.size() > 0)
            {
                std::tm timeS;
                memset(&timeS, 0, sizeof(std::tm));
                if (sscanf(all_images[0].time_coverage_start.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d.%*dZ", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
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
            std::shared_ptr<satdump::products::ImageProduct> abi_products = std::make_shared<satdump::products::ImageProduct>();
            abi_products->instrument_name = "abi";

            size_t largest_width = 0, largest_height = 0;
            ParsedGOESRABI largestImg;
            for (auto &img : all_images)
            {
                abi_products->images.push_back({img.channel - 1, "ABI-" + std::to_string(img.channel), std::to_string(img.channel), img.img, 14});

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

                center_longitude = img.longitude;
            }

            abi_products->set_product_timestamp(prod_timestamp);
            abi_products->set_product_source(satellite);

            {
                nlohmann::json proj_cfg;

                double line_scalar = std::abs(largestImg.perspective_point_height * largestImg.y_scale_factor);
                double line_offset = -largestImg.y_add_offset / largestImg.y_scale_factor;
                double column_scalar = std::abs(largestImg.perspective_point_height * largestImg.x_scale_factor);
                double column_offset = -largestImg.x_add_offset / largestImg.x_scale_factor;

                double scalar_x = column_scalar;
                double scalar_y = line_scalar;

                for (auto &img : abi_products->images)
                    img.ch_transform = satdump::ChannelTransform().init_affine(double(largest_width) / img.image.width(), double(largest_height) / img.image.height(), 0, 0);

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
                abi_products->set_proj_cfg(proj_cfg);
            }

            const float goes_abi_wavelength_table[16] = {
                470,   //
                640,   //
                860,   //
                1380,  //
                1610,  //
                2260,  //
                3900,  //
                6190,  //
                6950,  //
                7340,  //
                8500,  //
                9610,  //
                10350, //
                11200, //
                12300, //
                13300, //
            }; // TODOREWORK MOVE TO FILE!

            nlohmann::json calib_cfg;
            for (size_t i = 0; i < all_images.size(); i++)
            {
                int channel_id = all_images[i].channel - 1;
                calib_cfg["vars"]["scale"][channel_id] = all_images[i].calibration_scale;
                calib_cfg["vars"]["offset"][channel_id] = all_images[i].calibration_offset;
                calib_cfg["vars"]["kappa"][channel_id] = all_images[i].kappa;
                calib_cfg["vars"]["is_spectral"] = false;
            }
            abi_products->set_calibration("goes_nc_abi", calib_cfg);

            for (int i = 0; i < (int)all_images.size(); i++)
            {
                int channel_id = all_images[i].channel - 1;

                if (all_images[i].kappa > 0)
                    abi_products->set_channel_unit(channel_id, CALIBRATION_ID_ALBEDO);
                else
                    abi_products->set_channel_unit(channel_id, CALIBRATION_ID_EMISSIVE_RADIANCE);

                abi_products->set_channel_wavenumber(channel_id, 1e7 / goes_abi_wavelength_table[channel_id]);
            }

            return abi_products;
        }
    } // namespace firstparty
} // namespace satdump