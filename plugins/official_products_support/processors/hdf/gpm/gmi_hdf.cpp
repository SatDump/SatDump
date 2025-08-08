#include "gmi_hdf.h"
#include "common/utils.h"
#include "image/image.h"
#include "logger.h"
#include "utils/string.h"
#include <H5Cpp.h>
#include <H5LTpublic.h>
#include <string>

namespace satdump
{
    namespace official
    {
        // TODOREWORK probably make this generic?
        std::vector<image::Image> get_multi_img_from_hdf_NUMLAST(H5::H5File &file, std::string path)
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
                        img.set(0, x, y, img.clamp(tmp_all_channels[(y * image_dims[1] + x) * image_dims[2] + i] * 100));
                imgs.push_back(img);
            }

            return imgs;
        }

        std::vector<float> get_2d_floats(H5::H5File &file, std::string path)
        {
            H5::DataSet s = file.openDataSet(path);

            if (s.getSpace().getSimpleExtentNdims() != 2)
            {
                logger->error("Rank is not 2!");
                return {};
            }

            hsize_t image_dims[2];
            s.getSpace().getSimpleExtentDims(image_dims);

            std::vector<float> vals(image_dims[0] * image_dims[1], 0);
            s.read(vals.data(), H5::PredType::NATIVE_FLOAT);

            return vals;
        }

        void GMIHdfProcessor::ingestFile(std::vector<uint8_t> vec)
        {
            H5::H5File file(H5LTopen_file_image(vec.data(), vec.size(), H5F_ACC_RDONLY));

            gmi_products = std::make_shared<satdump::products::ImageProduct>();
            gmi_products->instrument_name = "gpm_gmi";
            // gmi_products->set_product_timestamp(timestamp);
            // gmi_products->set_product_source(satellite_name);

            auto imgs_S1 = get_multi_img_from_hdf_NUMLAST(file, "S1/Tb");
            auto imgs_S2 = get_multi_img_from_hdf_NUMLAST(file, "S2/Tb");

            for (int i = 0; i < 9; i++)
                gmi_products->images.push_back({i, "GMI-" + std::to_string(i + 1), std::to_string(i + 1), imgs_S1[i], 16, satdump::ChannelTransform().init_none()});
            for (int i = 0; i < 4; i++)
                gmi_products->images.push_back({i + 9, "GMI-" + std::to_string(i + 10), std::to_string(i + 10), imgs_S2[i], 16, satdump::ChannelTransform().init_none()});

            auto lat = get_2d_floats(file, "S1/Latitude");
            auto lon = get_2d_floats(file, "S1/Longitude");

            nlohmann::json gcps_all;
            int all_gcps = 0;

            for (int x = 0; x < 221; x++)
            {
                for (int y = 0; y < lat.size() / 221; y++)
                {
                    gcps_all[all_gcps]["x"] = x;
                    gcps_all[all_gcps]["y"] = y;
                    gcps_all[all_gcps]["lat"] = lat[y * 221 + x];
                    gcps_all[all_gcps]["lon"] = lon[y * 221 + x];
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
            gmi_products->set_proj_cfg(proj_cfg);
        }

        std::shared_ptr<satdump::products::Product> GMIHdfProcessor::getProduct() { return gmi_products; }
    } // namespace official
} // namespace satdump