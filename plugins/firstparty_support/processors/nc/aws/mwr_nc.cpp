#include "mwr_nc.h"
#include "common/utils.h"
#include "image/image.h"
#include "logger.h"
#include "utils/string.h"
#include <H5Cpp.h>
#include <H5LTpublic.h>
#include <cstdint>
#include <string>

namespace satdump
{
    namespace firstparty
    {
        std::vector<image::Image> get_multi_img_from_hdf_NUMLAST2(H5::H5File &file, std::string path)
        {
            H5::DataSet s = file.openDataSet(path);

            if (s.getSpace().getSimpleExtentNdims() != 3)
            {
                logger->error("Rank is not 3!");
                return {};
            }

            hsize_t image_dims[3];
            s.getSpace().getSimpleExtentDims(image_dims);

            std::vector<float> tmp_all_channels(image_dims[0] * image_dims[1] * image_dims[2], 0);
            s.read(tmp_all_channels.data(), H5::PredType::NATIVE_FLOAT);

            std::vector<image::Image> imgs;
            for (int i = 0; i < image_dims[2]; i++)
            {
                image::Image img(16, image_dims[1], image_dims[0], 1);
                for (int x = 0; x < img.width(); x++)
                    for (int y = 0; y < img.height(); y++)
                        img.set(0, x, y, img.clamp(tmp_all_channels[(y * image_dims[1] + x) * image_dims[2] + i] * 0.001 * 100));
                imgs.push_back(img);
            }

            return imgs;
        }

        // TODOREWORK probably make this generic?
        std::vector<image::Image> get_multi_img_from_hdf_NUMLAST_uint16_t(H5::H5File &file, std::string path)
        {
            H5::DataSet s = file.openDataSet(path);

            if (s.getSpace().getSimpleExtentNdims() != 3)
            {
                logger->error("Rank is not 3!");
                return {};
            }

            hsize_t image_dims[3];
            s.getSpace().getSimpleExtentDims(image_dims);

            std::vector<uint16_t> tmp_all_channels(image_dims[0] * image_dims[1] * image_dims[2], 0);
            s.read(tmp_all_channels.data(), H5::PredType::NATIVE_UINT16);

            std::vector<image::Image> imgs;
            for (int i = 0; i < image_dims[2]; i++)
            {
                image::Image img(16, image_dims[1], image_dims[0], 1);
                for (int x = 0; x < img.width(); x++)
                    for (int y = 0; y < img.height(); y++)
                        img.set(0, x, y, img.clamp(tmp_all_channels[(y * image_dims[1] + x) * image_dims[2] + i]));
                imgs.push_back(img);
            }

            return imgs;
        }

        // std::vector<float> get_2d_floats(H5::H5File &file, std::string path)
        // {
        //     H5::DataSet s = file.openDataSet(path);

        //     if (s.getSpace().getSimpleExtentNdims() != 2)
        //     {
        //         logger->error("Rank is not 2!");
        //         return {};
        //     }

        //     hsize_t image_dims[2];
        //     s.getSpace().getSimpleExtentDims(image_dims);

        //     std::vector<float> vals(image_dims[0] * image_dims[1], 0);
        //     s.read(vals.data(), H5::PredType::NATIVE_FLOAT);

        //     return vals;
        // }

        std::vector<float> get_2d_floats_firstch(H5::H5File &file, std::string path)
        {
            H5::DataSet s = file.openDataSet(path);

            if (s.getSpace().getSimpleExtentNdims() != 3)
            {
                logger->error("Rank is not 3!");
                return {};
            }

            hsize_t image_dims[3];
            s.getSpace().getSimpleExtentDims(image_dims);

            std::vector<double> vals(image_dims[0] * image_dims[1] * image_dims[2], 0);
            s.read(vals.data(), H5::PredType::NATIVE_DOUBLE);

            std::vector<float> valsf(image_dims[0] * image_dims[1], 0);
            for (int x = 0; x < image_dims[1]; x++)
                for (int y = 0; y < image_dims[0]; y++)
                    valsf[y * image_dims[1] + x] = vals[(y * image_dims[1] + x) * 4] * 1.0E-4;

            return valsf;
        }

        void MWRNcProcessor::ingestFile(std::vector<uint8_t> vec)
        {
            H5::H5File file(H5LTopen_file_image(vec.data(), vec.size(), H5F_ACC_RDONLY));

            mwr_products = std::make_shared<satdump::products::ImageProduct>();
            mwr_products->instrument_name = "aws_mwr";
            // gmi_products->set_product_timestamp(timestamp);
            // gmi_products->set_product_source(satellite_name);

            // auto imgs = get_multi_img_from_hdf_NUMLAST_uint16(file, "data/measurement/aws_earth_view_counts");
            auto imgs = get_multi_img_from_hdf_NUMLAST2(file, "data/calibration/aws_toa_brightness_temperature");

            // TODOREWORK Make Common
            // Channels are aligned by groups. 1 to 8 / 9 / 10 to 15 / 16 to 19
            satdump::ChannelTransform tran[19];
            double halfscan = 145.0 / 2.0;
            for (auto &v : tran)
                v.init_none();
            tran[8].init_affine_slantx(1, 1, -6.5, 0, halfscan, 11.0 / halfscan);
            tran[10] = tran[11] = tran[12] = tran[13] = tran[14] = tran[9].init_affine_slantx(0.92, 1, -0.5, 5, halfscan, 11.0 / halfscan);
            tran[15] = tran[16] = tran[17] = tran[18].init_affine_slantx(0.93, 1, 2.8, 4, halfscan, 6.0 / halfscan);

            for (int i = 0; i < 19; i++)
            {
                imgs[i].mirror(true, false);
                mwr_products->images.push_back({i, "MWR-" + std::to_string(i + 1), std::to_string(i + 1), imgs[i], 16, tran[i]});
            }

            auto lat = get_2d_floats_firstch(file, "data/navigation/aws_lat");
            auto lon = get_2d_floats_firstch(file, "data/navigation/aws_lon");

            nlohmann::json gcps_all;
            int all_gcps = 0;

            for (int x = 0; x < 145; x += 5)
            {
                for (int y = 0; y < lat.size() / 145; y++)
                {
                    gcps_all[all_gcps]["x"] = 144 - x;
                    gcps_all[all_gcps]["y"] = y;
                    gcps_all[all_gcps]["lat"] = lat[y * 145 + x];
                    gcps_all[all_gcps]["lon"] = lon[y * 145 + x];
                    all_gcps++;
                }
            }

            nlohmann::json proj_cfg; // = p;
            proj_cfg["type"] = "gcps_timestamps_line";
            proj_cfg["gcp_cnt"] = all_gcps;
            proj_cfg["gcps"] = gcps_all;
            // proj_cfg["timestamps"] = timestamps;
            proj_cfg["gcp_spacing_x"] = 10;
            proj_cfg["gcp_spacing_y"] = 10;
            mwr_products->set_proj_cfg(proj_cfg);

            // TODOREWORK
            double mwr_freqs[19] = {
                50.3e9, 52.8e9, 53.246e9, 53.596e9, 54.4e9, 54.94e9, 55.5e9, 57.290344e9, 89e9, 165.5e9, 176.311e9, 178.811e9, 180.311e9, 181.511e9, 182.311e9, 325.15e9, 325.15e9, 325.15e9, 325.15e9,
            };

            for (int i = 0; i < 19; i++)
            {
                mwr_products->set_channel_frequency(i, mwr_freqs[i]);
                mwr_products->set_channel_unit(i, CALIBRATION_ID_EMISSIVE_RADIANCE);
            }

            mwr_products->set_calibration("aws_mwr_nc", {});
        }

        std::shared_ptr<satdump::products::Product> MWRNcProcessor::getProduct() { return mwr_products; }
    } // namespace firstparty
} // namespace satdump