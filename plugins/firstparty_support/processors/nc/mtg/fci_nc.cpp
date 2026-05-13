#include "fci_nc.h"
#include "../../hdf_utils.h"
#include "H5DataSpace.h"
#include "core/resources.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "projection/standard/proj_json.h"
#include <H5Cpp.h>
#include <H5LTpublic.h>

extern "C"
{
    void register_MTG_FILTER();
}

namespace satdump
{
    namespace firstparty
    {
        FCINcProcessor::ParsedMTGFCI FCINcProcessor::parse_mtg_fci_netcdf_fulldisk(std::vector<uint8_t> data)
        {
            ParsedMTGFCI image_out;

            hsize_t image_dims[2];

            H5::H5File file(H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY));

            bool is_hr_mode = true;

            {
                H5::DataSet s = file.openDataSet("l1c_channels_present");
                hsize_t l1c_channels[2];
                s.getSpace().getSimpleExtentDims(l1c_channels);
                is_hr_mode = l1c_channels[0] == 4;
            }

            image_out.platform_name = hdfpp_read_attribute_string(file.openAttribute("platform"));
            image_out.time_coverage_start = hdfpp_read_attribute_string(file.openAttribute("time_coverage_start"));
            image_out.longitude = hdfpp_read_attribute_double(file.openDataSet("data/mtg_geos_projection").openAttribute("longitude_of_projection_origin"));

            const std::string channel_map[16] = {
                "vis_04", "vis_05", "vis_06", "vis_08", "vis_09", "nir_13", "nir_16", "nir_22", "ir_38", "wv_63", "wv_73", "ir_87", "ir_97", "ir_105", "ir_123", "ir_133",
            };

            const std::string channel_map_hr[16] = {
                "_", "_", "vis_06_hr", "_", "_", "_", "_", "nir_22_hr", "ir_38_hr", "_", "_", "_", "_", "ir_105_hr", "_", "_",
            };

            for (int ch = 0; ch < 16; ch++)
            {
                auto test_path = (std::string) "data/" + std::string(is_hr_mode ? channel_map_hr[ch] : channel_map[ch]);
                auto channel_path = (std::string) "data/" + std::string(is_hr_mode ? channel_map_hr[ch] : channel_map[ch]) + "/measured/effective_radiance";
                auto rowoff_path = (std::string) "data/" + std::string(is_hr_mode ? channel_map_hr[ch] : channel_map[ch]) + "/measured/start_position_row";

                if (!file.nameExists(test_path.c_str()))
                {
                    logger->trace("Skipping channel %d", ch + 1);
                    continue;
                }

                image_out.calibration_scale[ch] = hdfpp_read_attribute_double(file.openDataSet(channel_path).openAttribute("scale_factor"));
                image_out.calibration_offset[ch] = hdfpp_read_attribute_double(file.openDataSet(channel_path).openAttribute("add_offset"));

                {
                    int start_row = get_one_int_dataset(file, rowoff_path);
                    if (start_row == -1e6)
                        continue;
                    image_out.start_row[ch] = start_row;
                }

                auto dataset = file.openDataSet(channel_path);

                int rank = dataset.getSpace().getSimpleExtentNdims();
                dataset.getSpace().getSimpleExtentDims(image_dims);

                if (rank != 2)
                    return image_out;

                image::Image img(16, image_dims[1], image_dims[0], 1);
                dataset.read(img.raw_data(), H5::PredType::NATIVE_UINT16, H5::DataSpace(2, image_dims));

                for (size_t i = 0; i < img.size(); i++)
                    if (img.get(i) == 65535)
                        img.set(i, 0);
                    else
                        img.set(i, img.get(i) << 4);

                image_out.imgs[ch] = img;
            }

            return image_out;
        }

        void FCINcProcessor::ingestFile(std::vector<uint8_t> vec)
        {
            register_MTG_FILTER();

            auto img = parse_mtg_fci_netcdf_fulldisk(vec);
            all_images.push_back(img);

            if (img.platform_name == "MTI1")
                satellite = "MTG-I1";
            else if (img.platform_name == "MTI2")
                satellite = "MTG-I2";

            std::tm timeS;
            memset(&timeS, 0, sizeof(std::tm));
            if (sscanf(img.time_coverage_start.c_str(), "%4d%2d%2d%2d%2d%2d", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
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
        }

        std::shared_ptr<satdump::products::Product> FCINcProcessor::getProduct()
        {
            if (prod_timestamp == 0)
                prod_timestamp = time(0);

            image::Image final_imgs[16];
            bool final_imgs_present[16] = {false};

            // Saving
            std::shared_ptr<satdump::products::ImageProduct> fci_products = std::make_shared<satdump::products::ImageProduct>();
            fci_products->instrument_name = "fci";
            fci_products->set_product_timestamp(prod_timestamp);
            fci_products->set_product_source(satellite);

            int largest_width = 0;
            int largest_height = 0;

            for (int i = 0; i < 16; i++)
            {
                int final_x = 0, final_y = 0;
                for (auto &img : all_images)
                {
                    final_x = img.imgs[i].width();
                    final_y += img.imgs[i].height();
                }

                // Temporary?
                final_y = final_x;

                final_imgs[i] = image::Image(16, final_x, final_y, 1);

                if (final_x > largest_width)
                    largest_width = final_x;
                if (final_y > largest_height)
                    largest_height = final_y;

                for (auto &img : all_images)
                {
                    if (img.imgs[i].depth() == final_imgs[i].depth())
                    {
                        final_imgs[i].draw_image(0, img.imgs[i], 0, img.start_row[i]);
                        final_imgs_present[i] = true;
                    }
                    img.imgs[i].clear();
                }

                final_imgs[i].mirror(false, true);

                if (final_imgs_present[i])
                    fci_products->images.push_back({i, "FCI-" + std::to_string(i + 1), std::to_string(i + 1), final_imgs[i], 12,
                                                    satdump::ChannelTransform().init_affine(3712.0 / final_imgs[i].width(), 3712.0 / final_imgs[i].height(), 0, 0)});

                logger->info("Processing channel %d", i + 1);
                final_imgs[i].clear();
            }

            proj::projection_t proj;
            proj.type = proj::ProjType_Geos;
            proj.params.sweep_x = false;
            proj.params.altitude = 3.57864e7;
            proj.proj_offset_x = -5568748.275756353;
            proj.proj_offset_y = 5568748.275756353;
            proj.proj_scalar_x = 3000.403165817;  // / double(largest_width));
            proj.proj_scalar_y = -3000.403165817; // / double(largest_height));
            proj.lam0 = center_longitude * DEG2RAD;
            nlohmann::json proj_cfg = proj;
            proj_cfg["width"] = 3712;
            proj_cfg["height"] = 3712; // 3712;
            fci_products->set_proj_cfg(proj_cfg);

            nlohmann::json fci_config = loadJsonFile(resources::getResourcePath("calibration/FCI_table.json"));

            nlohmann::json calib_cfg;
            for (int i = 0; i < 16; i++)
            {
                calib_cfg["vars"]["scale"][i] = calibration_scale[i];
                calib_cfg["vars"]["offset"][i] = calibration_offset[i];
            }
            fci_products->set_calibration("mtg_nc_fci", calib_cfg);

            for (int i = 0; i < 16; i++)
            {
                if (final_imgs_present[i])
                {
                    if (i < 8)
                        fci_products->set_channel_unit(i, CALIBRATION_ID_REFLECTIVE_RADIANCE);
                    else
                        fci_products->set_channel_unit(i, CALIBRATION_ID_EMISSIVE_RADIANCE);

                    fci_products->set_channel_wavenumber(i, freq_to_wavenumber(299792458.0 / (fci_config["wavelengths"][i].get<double>())));
                }
            }

            return fci_products;
        }
    } // namespace firstparty
} // namespace satdump