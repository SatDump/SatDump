#include "sc3_olci.h"

#include "common/utils.h"
#include "image/image.h"
#include <filesystem>
#include "logger.h"
#include <array>
#include "products2/image_product.h"
#include "libs/miniz/miniz.h"

#include "common/calibration.h"

#include "nlohmann/json_utils.h"
#include "core/resources.h"
#include "hdf5_utils.h"
#include <H5LTpublic.h>

namespace nc2pro
{
    struct ParseOLCIChannel
    {
        image::Image img;
        std::string start_time;
        std::string satellite_id;
    };

    ParseOLCIChannel parse_sentinel3_ocli_channel(std::vector<uint8_t> data, std::string channel)
    {
        ParseOLCIChannel o;

        hsize_t image_dims[2];

        hid_t file = H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY);

        if (file < 0)
            return o;

        o.start_time = hdf5_get_string_attr_FILE_fixed(file, "start_time");
        o.satellite_id = hdf5_get_string_attr_FILE_fixed(file, "product_name");

        hid_t dataset = H5Dopen2(file, std::string("Oa" + channel + "_radiance").c_str(), H5P_DEFAULT);

        if (dataset < 0)
            return o;

        hid_t dataspace = H5Dget_space(dataset);
        int rank = H5Sget_simple_extent_ndims(dataspace);
        H5Sget_simple_extent_dims(dataspace, image_dims, NULL);

        if (rank != 2)
            return o;

        hid_t memspace = H5Screate_simple(2, image_dims, NULL);

        o.img = image::Image(16, image_dims[1], image_dims[0], 1);

        H5Dread(dataset, H5T_NATIVE_UINT16, memspace, dataspace, H5P_DEFAULT, (uint16_t *)o.img.raw_data());

        for (size_t i = 0; i < o.img.size(); i++)
            if (o.img.get(i) == 65535)
                o.img.set(i, 0);

        H5Dclose(dataset);
        H5Fclose(file);

        return o;
    }

    struct GCPCont
    {
        int x = 0;
        int y = 0;
        nlohmann::json v;
    };

    GCPCont parse_sentinel3_ocli_geo(std::vector<uint8_t> data)
    {
        hsize_t image_dims[2];

        hid_t file = H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY);

        int all_gcps = 0;
        nlohmann::json gcps;

        if (file < 0)
            return {0, 0, gcps};

        int dim_x = -1, dim_y = -1;
        std::vector<int> lat, lon;

        for (int l = 0; l < 2; l++)
        {
            hid_t dataset = H5Dopen2(file, l == 0 ? "latitude" : "longitude", H5P_DEFAULT);

            if (dataset < 0)
                return {0, 0, gcps};

            hid_t dataspace = H5Dget_space(dataset);
            int rank = H5Sget_simple_extent_ndims(dataspace);
            H5Sget_simple_extent_dims(dataspace, image_dims, NULL);

            if (rank != 2)
                return {0, 0, gcps};

            hid_t memspace = H5Screate_simple(2, image_dims, NULL);

            std::vector<int> &cur = l == 0 ? lat : lon;
            cur.resize(image_dims[0] * image_dims[1]);

            dim_x = image_dims[1];
            dim_y = image_dims[0];

            H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, cur.data());

            H5Dclose(dataset);
        }

        for (int x = 0; x < dim_x; x += (dim_x / 50))
        {
            for (int y = 0; y < dim_y; y += 1) //(dim_y / 50))
            {
                gcps[all_gcps]["x"] = x;
                gcps[all_gcps]["y"] = y;
                gcps[all_gcps]["lat"] = lat[y * dim_x + x] * 1e-6;
                gcps[all_gcps]["lon"] = lon[y * dim_x + x] * 1e-6;
                all_gcps++;
            }
        }

        H5Fclose(file);

        return {dim_x, dim_y, gcps};
    }

    void process_sc3_ocli(std::string zip_file, std::string pro_output_file, double *progess)
    {
        image::Image all_channels[21];

        time_t prod_timestamp = time(0);
        std::string satellite = "Unknown Sentinel-3";

        GCPCont gcps_all;

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

                        int chnum = 0;
                        if (sscanf(name.c_str(), "Oa%2d_radiance", &chnum) == 1 && chnum <= 21 && name.find("_unc") == std::string::npos)
                        {
                            size_t filesize = 0;
                            void *file_ptr = mz_zip_reader_extract_to_heap(&zip, fi, &filesize, 0);

                            std::vector<uint8_t> vec((uint8_t *)file_ptr, (uint8_t *)file_ptr + filesize);
                            auto img = parse_sentinel3_ocli_channel(vec, (chnum < 10 ? "0" : "") + std::to_string(chnum));
                            all_channels[chnum - 1] = img.img;

                            if (img.satellite_id.find("S3A") != std::string::npos)
                                satellite = "Sentinel-3A";
                            else if (img.satellite_id.find("S3B") != std::string::npos)
                                satellite = "Sentinel-3B";
                            else if (img.satellite_id.find("S3C") != std::string::npos)
                                satellite = "Sentinel-3C";

                            std::tm timeS;
                            memset(&timeS, 0, sizeof(std::tm));
                            if (sscanf(img.start_time.c_str(),
                                       "%4d-%2d-%2dT%2d:%2d:%2d.%*dZ",
                                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
                            {
                                timeS.tm_year -= 1900;
                                timeS.tm_mon -= 1;
                                timeS.tm_isdst = -1;
                                prod_timestamp = timegm(&timeS);
                            }

                            mz_free(file_ptr);
                        }
                        else if (name == "geo_coordinates")
                        {
                            size_t filesize = 0;
                            void *file_ptr = mz_zip_reader_extract_to_heap(&zip, fi, &filesize, 0);
                            std::vector<uint8_t> vec((uint8_t *)file_ptr, (uint8_t *)file_ptr + filesize);
                            gcps_all = parse_sentinel3_ocli_geo(vec);
                            mz_free(file_ptr);
                        }
                    }
                }

                *progess = double(fi) / double(numfiles);
            }

            mz_zip_reader_end(&zip);
        }

        // Saving
        satdump::products::ImageProduct olci_products;
        olci_products.instrument_name = "olci";
        olci_products.set_product_timestamp(prod_timestamp);
        olci_products.set_product_source(satellite);

        double ratio_x1 = gcps_all.x != 0 ? gcps_all.x : double(all_channels[0].width());
        for (int i = 0; i < 21; i++)
        {
            double ratio = ratio_x1 / double(all_channels[i].width());
            olci_products.images.push_back({i, "OLCI-" + std::to_string(i + 1), std::to_string(i + 1), all_channels[i], 16,
                                            satdump::ChannelTransform().init_affine(ratio, ratio, 0, 0)});
        }

#if 1
        nlohmann::json proj_cfg;
        proj_cfg["type"] = "gcps_timestamps_line";
        proj_cfg["gcp_cnt"] = gcps_all.v.size();
        proj_cfg["gcps"] = gcps_all.v;
        proj_cfg["timestamps"] = {0, 1}; // TODOREWORK MAKE THESE WORK!
        proj_cfg["gcp_spacing_x"] = 100;
        proj_cfg["gcp_spacing_y"] = 100;
        olci_products.set_proj_cfg(proj_cfg);
#endif

        if (!std::filesystem::exists(pro_output_file))
            std::filesystem::create_directories(pro_output_file);
        olci_products.save(pro_output_file);
    }
}
