#include "ssmis_hdf.h"
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
        std::vector<image::Image> get_multi_img_from_hdf_NUMLAST(H5::H5File &file, std::string path);

        std::vector<float> get_2d_floats(H5::H5File &file, std::string path);

        void SSMISHdfProcessor::ingestFile(std::vector<uint8_t> vec)
        {
            H5::H5File file(H5LTopen_file_image(vec.data(), vec.size(), H5F_ACC_RDONLY));

            ssmis_products = std::make_shared<satdump::products::ImageProduct>();
            ssmis_products->instrument_name = "ssmis";
            // ssmis_products->set_product_timestamp(timestamp);
            // ssmis_products->set_product_source(satellite_name);

            auto imgs_S1 = get_multi_img_from_hdf_NUMLAST(file, "S1/Tc");
            auto imgs_S2 = get_multi_img_from_hdf_NUMLAST(file, "S2/Tc");
            auto imgs_S3 = get_multi_img_from_hdf_NUMLAST(file, "S3/Tc");
            auto imgs_S4 = get_multi_img_from_hdf_NUMLAST(file, "S4/Tc");

            for (int i = 0; i < 3; i++)
                ssmis_products->images.push_back({i, "SSMIS-" + std::to_string(i + 1), std::to_string(i + 1), imgs_S1[i], 16, satdump::ChannelTransform().init_affine(2, 1, 0, 0)});
            for (int i = 3; i < 5; i++)
                ssmis_products->images.push_back({i, "SSMIS-" + std::to_string(i + 1), std::to_string(i + 1), imgs_S2[i - 3], 16, satdump::ChannelTransform().init_affine(2, 1, 0, 0)});
            for (int i = 5; i < 9; i++)
                ssmis_products->images.push_back({i, "SSMIS-" + std::to_string(i + 1), std::to_string(i + 1), imgs_S3[i - 5], 16, satdump::ChannelTransform().init_none()});
            for (int i = 9; i < 11; i++)
                ssmis_products->images.push_back({i, "SSMIS-" + std::to_string(i + 1), std::to_string(i + 1), imgs_S4[i - 9], 16, satdump::ChannelTransform().init_none()});

            // S1
            ssmis_products->set_channel_frequency(0, 19.35e9);
            ssmis_products->set_channel_frequency(1, 19.35e9);
            ssmis_products->set_channel_frequency(2, 22.235e9);

            // S2
            ssmis_products->set_channel_frequency(3, 37e9);
            ssmis_products->set_channel_frequency(4, 37e9);

            // S3
            ssmis_products->set_channel_frequency(5, 150e9);
            ssmis_products->set_channel_frequency(6, 183.31e9);
            ssmis_products->set_channel_frequency(7, 183.31e9);
            ssmis_products->set_channel_frequency(8, 183.31e9);

            // S4
            ssmis_products->set_channel_frequency(9, 91.655e9);
            ssmis_products->set_channel_frequency(10, 91.655e9);

            for (int i = 0; i < 11; i++)
                ssmis_products->set_channel_unit(i, CALIBRATION_ID_EMISSIVE_RADIANCE);
            ssmis_products->set_calibration("gpm_hdf", {});

            auto lat = get_2d_floats(file, "S4/Latitude");
            auto lon = get_2d_floats(file, "S4/Longitude");

            nlohmann::json gcps_all;
            int all_gcps = 0;

            for (int x = 0; x < 180; x++)
            {
                for (int y = 0; y < lat.size() / 180; y++)
                {
                    gcps_all[all_gcps]["x"] = x;
                    gcps_all[all_gcps]["y"] = y;
                    gcps_all[all_gcps]["lat"] = lat[y * 180 + x];
                    gcps_all[all_gcps]["lon"] = lon[y * 180 + x];
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
            ssmis_products->set_proj_cfg(proj_cfg);
        }

        std::shared_ptr<satdump::products::Product> SSMISHdfProcessor::getProduct() { return ssmis_products; }
    } // namespace official
} // namespace satdump