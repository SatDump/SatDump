#include "agri_hdf.h"
#include "../../hdf_utils.h"
#include "image/image.h"
#include "logger.h"
#include "projection/standard/proj_json.h"
#include <H5Cpp.h>
#include <H5LTpublic.h>
#include <cstdint>
#include <exception>
#include <string>

#ifdef _WIN32
#define timegm _mkgmtime
#endif

namespace satdump
{
    namespace firstparty
    {
        // TODOREWORK probably make this generic?
        std::vector<image::Image> get_multi_img_from_hdf(H5::H5File &file, std::string path);

        image::Image get_img_from_hdf(H5::H5File &file, std::string path);

        void AGRIHdfProcessor::ingestFile(std::vector<uint8_t> vec)
        {
            H5::H5File file(H5LTopen_file_image(vec.data(), vec.size(), H5F_ACC_RDONLY));

            double timestamp = -1;
            {
                std::string date_str;
                auto att = file.openAttribute("Observing Beginning Date");
                att.read(att.getStrType(), date_str);

                std::string time_str;
                auto att2 = file.openAttribute("Observing Beginning Time");
                att2.read(att2.getStrType(), time_str);

                std::tm timeS;
                memset(&timeS, 0, sizeof(std::tm));
                if (sscanf(date_str.c_str(), "%4d-%2d-%2d", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday) != 3)
                    logger->error("Parse Date Error!");
                if (sscanf(time_str.c_str(), "%2d:%2d:%2d", &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) != 3)
                    logger->error("Parse Time Error!");
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                timestamp = timegm(&timeS);
            }

            std::string name_str;
            auto att = file.openAttribute("Satellite Name");
            att.read(att.getStrType(), name_str);

            // Init as needed
            if (!agri_products)
            {
                agri_products = std::make_shared<satdump::products::ImageProduct>();
                agri_products->instrument_name = "agri";
                agri_products->set_product_timestamp(timestamp);
                agri_products->set_product_source(name_str);
            }

            sat_lon = hdfpp_read_attribute_double(file.openAttribute("NOMCenterLon"));
            sat_height = hdfpp_read_attribute_double(file.openAttribute("NOMSatHeight"));

            // There, we want only Data
            for (int i = 0; i < 14; i++)
            {
                std::string name = i < 10 ? ("NOMChannel0" + std::to_string(i + 1)) : ("NOMChannel" + std::to_string(i + 1));
                if (file.nameExists(name))
                {
                    auto img = get_img_from_hdf(file, name);
                    for (size_t i = 0; i < img.size(); i++)
                        img.set(i, img.get(i) << 4);

                    try
                    {
                        auto &h = agri_products->get_channel_image(i);
                        if (h.image.width() < img.width())
                            h.image = img;
                    }
                    catch (std::exception &e)
                    {
                        agri_products->images.push_back({i, "AGRI-" + std::to_string(i + 1), std::to_string(i + 1), img, 16, satdump::ChannelTransform().init_none()});
                    }
                }
            }
        }

        std::shared_ptr<satdump::products::Product> AGRIHdfProcessor::getProduct()
        {
            /*
            proj::projection_t proj;
            proj.type = proj::ProjType_Geos;
            proj.params.sweep_x = false;
            proj.params.altitude = sat_height; // 3.57864e7;
            proj.proj_offset_x = -5568748.275756353;
            proj.proj_offset_y = 5568748.275756353;
            proj.proj_scalar_x = 3000.403165817 * (3752. / 5496.);  // / double(largest_width));
            proj.proj_scalar_y = -3000.403165817 * (3752. / 5496.); // / double(largest_height));
            proj.lam0 = sat_lon * DEG2RAD;
            nlohmann::json proj_cfg = proj;
            proj_cfg["width"] = 5496;  // 3712;
            proj_cfg["height"] = 5496; // 3712; // 3712;
            agri_products->set_proj_cfg(proj_cfg);
            */

            return agri_products;
        }
    } // namespace firstparty
} // namespace satdump