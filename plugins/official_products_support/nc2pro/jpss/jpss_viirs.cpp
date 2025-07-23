#include "jpss_viirs.h"

#include "H5Tpublic.h"
#include "common/utils.h"
#include "image/image.h"
#include "libs/miniz/miniz.h"
#include "logger.h"
#include "products/image/channel_transform.h"
#include "products/image_product.h"
#include <array>
#include <filesystem>

#include "common/calibration.h"

#include "../hdf5_utils.h"
#include "core/resources.h"
#include "nlohmann/json_utils.h"
#include <H5LTpublic.h>
#include <string>
#include <vector>

#include "image/bowtie.h"
#include "utils/string.h"

namespace nc2pro
{
    struct ParsedJPSSChannels
    {
        std::vector<image::Image> img;
        std::vector<double> scale;
        std::vector<double> offset;
        std::string start_time;
        std::string satellite_id;
    };

    ParsedJPSSChannels parse_jpss_viirs_channels(std::vector<uint8_t> data, int mode)
    {
        ParsedJPSSChannels o;

        hsize_t image_dims[2];

        hid_t file = H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY);

        if (file < 0)
            return o;

        o.start_time = hdf5_get_string_attr_FILE_fixed(file, "StartTime");
        o.satellite_id = hdf5_get_string_attr_FILE_fixed(file, "platform");

        int numch = 0;
        if (mode == 0)
            numch = 5;
        else if (mode == 1)
            numch = 16;
        else if (mode == 2)
            numch = 1;

        o.img.resize(numch);
        o.scale.resize(numch);
        o.offset.resize(numch);

        for (int i = 0; i < numch; i++)
        {
            std::string ch_name;
            if (mode == 0)
                ch_name = "I0" + std::to_string(i + 1);
            else if (mode == 1)
                ch_name = i <= 8 ? ("M0" + std::to_string(i + 1)) : ("M" + std::to_string(i + 1));
            else if (mode == 2)
                ch_name = "DNB_observations";

            hid_t dataset = H5Dopen2(file, std::string("observation_data/" + ch_name).c_str(), H5P_DEFAULT);

            if (dataset < 0)
                return o;

            hid_t dataspace = H5Dget_space(dataset);
            int rank = H5Sget_simple_extent_ndims(dataspace);
            H5Sget_simple_extent_dims(dataspace, image_dims, NULL);

            if (rank != 2)
                return o;

            hid_t memspace = H5Screate_simple(2, image_dims, NULL);

            o.img[i] = image::Image(16, image_dims[1], image_dims[0], 1);

            H5Dread(dataset, H5T_NATIVE_UINT16, memspace, dataspace, H5P_DEFAULT, (uint16_t *)o.img[i].raw_data());

            o.scale[i] = hdf5_get_float_attr(file, std::string("observation_data/" + ch_name).c_str(), "radiance_scale_factor");
            o.offset[i] = hdf5_get_float_attr(file, std::string("observation_data/" + ch_name).c_str(), "radiance_add_offset");

            if (o.scale[i] == -1e6 || o.offset[i] == -1e6)
            {
                o.scale[i] = hdf5_get_float_attr(file, std::string("observation_data/" + ch_name).c_str(), "scale_factor");
                o.offset[i] = hdf5_get_float_attr(file, std::string("observation_data/" + ch_name).c_str(), "add_offset");
            }

            H5Dclose(dataset);
        }

        H5Fclose(file);

        return o;
    }

    struct GCPCont
    {
        int x = 0;
        int y = 0;
        nlohmann::json v;
    };

    GCPCont parse_jpss_viirs_geo(std::vector<uint8_t> data)
    {
        hsize_t image_dims[2];

        hid_t file = H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY);

        int all_gcps = 0;
        nlohmann::json gcps;

        if (file < 0)
            return {0, 0, gcps};

        int dim_x = -1, dim_y = -1;
        std::vector<float> lat, lon;

        for (int l = 0; l < 2; l++)
        {
            hid_t dataset = H5Dopen2(file, l == 0 ? "geolocation_data/latitude" : "geolocation_data/longitude", H5P_DEFAULT);

            if (dataset < 0)
                return {0, 0, gcps};

            hid_t dataspace = H5Dget_space(dataset);
            int rank = H5Sget_simple_extent_ndims(dataspace);
            H5Sget_simple_extent_dims(dataspace, image_dims, NULL);

            if (rank != 2)
                return {0, 0, gcps};

            hid_t memspace = H5Screate_simple(2, image_dims, NULL);

            std::vector<float> &cur = l == 0 ? lat : lon;
            cur.resize(image_dims[0] * image_dims[1]);

            dim_x = image_dims[1];
            dim_y = image_dims[0];

            H5Dread(dataset, H5T_NATIVE_FLOAT, memspace, dataspace, H5P_DEFAULT, cur.data());

            H5Dclose(dataset);
        }

        for (int x = 0; x < dim_x; x += 10) //(dim_x / 50))
        {
            for (int y = 0; y < dim_y; y += 1) //(dim_y / 50))
            {
                if (lat[y * dim_x + x] != -999.9f && lon[y * dim_x + x] != 999.9f)
                {
                    gcps[all_gcps]["x"] = x;
                    gcps[all_gcps]["y"] = y;
                    gcps[all_gcps]["lat"] = lat[y * dim_x + x];
                    gcps[all_gcps]["lon"] = lon[y * dim_x + x];
                    all_gcps++;
                }
            }
        }

        H5Fclose(file);

        return {dim_x, dim_y, gcps};
    }

    void process_jpss_viirs(std::string nc_file, std::string pro_output_file, double *progess)
    {
        time_t prod_timestamp = 0;
        std::string satellite = "Unknown JPSS";

        std::vector<std::string> files_to_parse;
        {
            auto ori_splt = satdump::splitString(std::filesystem::path(nc_file).stem().string(), '_');
            if (ori_splt.size() != 2)
                throw satdump_exception("Invalid VIIRS L1B filename!");

            std::filesystem::recursive_directory_iterator pluginIterator(std::filesystem::path(nc_file).parent_path());
            std::error_code iteratorError;
            while (pluginIterator != std::filesystem::recursive_directory_iterator())
            {
                std::string path = pluginIterator->path().string();
                if (!std::filesystem::is_regular_file(pluginIterator->path()) || pluginIterator->path().extension() != ".nc")
                    goto skip_this;

                {
                    std::string cleanname = std::filesystem::path(path).stem().string();
                    auto splt = satdump::splitString(cleanname, '_');

                    if (splt.size() == 2 && ori_splt[1] == splt[1])
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

        ParsedJPSSChannels viirs_img;
        ParsedJPSSChannels viirs_mod;
        ParsedJPSSChannels viirs_dnb;
        GCPCont viirs_gcps;

        for (auto &file : files_to_parse)
        {
            auto ori_splt = satdump::splitString(std::filesystem::path(file).stem().string(), '_');

            std::vector<uint8_t> nc_file;
            std::ifstream input_file(/*files_to_parse[i]*/ file, std::ios::binary);
            input_file.seekg(0, std::ios::end);
            const size_t nc_size = input_file.tellg();
            nc_file.resize(nc_size);
            input_file.seekg(0, std::ios::beg);
            input_file.read((char *)&nc_file[0], nc_size);
            input_file.close();

            if (ori_splt[0] == "VNP02IMG")
                viirs_img = parse_jpss_viirs_channels(nc_file, 0);
            else if (ori_splt[0] == "VNP02MOD")
                viirs_mod = parse_jpss_viirs_channels(nc_file, 1);
            // else if (ori_splt[0] == "VNP02DNB") // TODOREWORK.....
            //    viirs_dnb = parse_jpss_viirs_channels(nc_file, 2);
            // else if (ori_splt[0] == "VNP03IMG") // TODOREWORK.....
            //     viirs_gcps = parse_jpss_viirs_geo(nc_file);
        }

#if 1
        // Saving
        satdump::products::ImageProduct viirs_products;
        viirs_products.instrument_name = "viirs";
        viirs_products.set_product_timestamp(prod_timestamp);
        viirs_products.set_product_source(satellite);

        // Calib
        std::map<int, double> calibration_scale;
        std::map<int, double> calibration_offset;

        // BowTie values
        const float alpha = 1.0 / 1.9;
        const float beta = 0.52333; // 1.0 - alpha;

        // TODOREWORK put a in a file (and into DB stuff!)
        double wavelength_img[5] = {0.64e-6, 0.865e-6, 1.61e-6, 3.74e-6, 11.45e-6};
        double wavelength_mod[16] = {412e-9,  445e-9,  488e-9,    555e-9,   672e-9,  746e-9,  //
                                     865e-9,  1240e-9, 1378e-9,   1610e-9,  2250e-9, 3.70e-6, //
                                     4.05e-6, 8.55e-6, 10.763e-6, 12.013e-6};

        if (viirs_img.img.size())
        {
            for (int i = 0; i < 5; i++)
            {
                logger->info("I" + std::to_string(i + 1) + "...");
                viirs_img.img[i] = image::bowtie::correctGenericBowTie(viirs_img.img[i], 1, 32, alpha, beta);

                viirs_products.images.push_back({i, "VIIRS-I" + std::to_string(i + 1), "i" + std::to_string(i + 1), viirs_img.img[i], 16});

                if (i < 3)
                    viirs_products.set_channel_unit(i, CALIBRATION_ID_REFLECTIVE_RADIANCE);
                else
                    viirs_products.set_channel_unit(i, CALIBRATION_ID_EMISSIVE_RADIANCE);

                viirs_products.set_channel_wavenumber(i, freq_to_wavenumber(299792458.0 / wavelength_img[i]));
                calibration_scale.emplace(i, viirs_img.scale[i]);
                calibration_offset.emplace(i, viirs_img.offset[i]);
            }
        }

        if (viirs_mod.img.size())
        {
            for (int i = 0; i < 16; i++)
            {
                logger->info("M" + std::to_string(i + 1) + "...");
                viirs_mod.img[i] = image::bowtie::correctGenericBowTie(viirs_mod.img[i], 1, 16, alpha, beta);

                viirs_products.images.push_back({i + 5, "VIIRS-M" + std::to_string(i + 1), "m" + std::to_string(i + 1), viirs_mod.img[i], 16, satdump::ChannelTransform().init_affine(2, 2, 0, 0)});

                if (i <= 10)
                    viirs_products.set_channel_unit(i + 5, CALIBRATION_ID_REFLECTIVE_RADIANCE);
                else
                    viirs_products.set_channel_unit(i + 5, CALIBRATION_ID_EMISSIVE_RADIANCE);

                viirs_products.set_channel_wavenumber(i + 5, freq_to_wavenumber(299792458.0 / wavelength_mod[i]));
                calibration_scale.emplace(i + 5, viirs_mod.scale[i]);
                calibration_offset.emplace(i + 5, viirs_mod.offset[i]);
            }
        }

        if (viirs_dnb.img.size())
        {
            logger->info("DNB...");

            viirs_products.images.push_back({21, "VIIRS-DNB", "dnb", viirs_dnb.img[0], 16}); //, satdump::ChannelTransform().init_affine(2, 2, 0, 0)});

            calibration_scale.emplace(21, viirs_dnb.scale[0]);
            calibration_offset.emplace(21, viirs_dnb.offset[0]);
        }

        nlohmann::json calib_cfg;
        calib_cfg["vars"]["scale"] = calibration_scale;
        calib_cfg["vars"]["offset"] = calibration_offset;
        viirs_products.set_calibration("jpss_nc_viirs", calib_cfg);

        if (viirs_gcps.v.size())
        {
            nlohmann::json proj_cfg;
            proj_cfg["type"] = "gcps_timestamps_line";
            proj_cfg["gcp_cnt"] = viirs_gcps.v.size();
            proj_cfg["gcps"] = viirs_gcps.v;
            proj_cfg["timestamps"] = {0, 1}; // TODOREWORK MAKE THESE WORK!
            proj_cfg["gcp_spacing_x"] = 100;
            proj_cfg["gcp_spacing_y"] = 100;
            viirs_products.set_proj_cfg(proj_cfg);
        }

        if (!std::filesystem::exists(pro_output_file))
            std::filesystem::create_directories(pro_output_file);
        viirs_products.save(pro_output_file);
#endif
    }
} // namespace nc2pro
