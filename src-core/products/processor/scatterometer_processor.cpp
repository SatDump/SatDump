#include "processor.h"

#include "logger.h"
#include "../scatterometer_products.h"
#include "core/config.h"

#include "resources.h"

#include "common/projection/reprojector.h"
#include "common/map/map_drawer.h"

#include "common/image2/io/io.h"

namespace satdump
{
    void process_scatterometer_products(Products *products, std::string product_path)
    {
        ScatterometerProducts *rad_products = (ScatterometerProducts *)products;

        // Get instrument settings
        nlohmann::ordered_json instrument_viewer_settings;
        if (config::main_cfg["viewer"]["instruments"].contains(products->instrument_name))
            instrument_viewer_settings = config::main_cfg["viewer"]["instruments"][products->instrument_name];
        else
            logger->error("Unknown instrument : %s!", products->instrument_name.c_str());

        if (instrument_viewer_settings.contains("grayscale_images"))
        {
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_viewer_settings["grayscale_images"].items())
            {
                std::string initial_name = compo.key();
                std::replace(initial_name.begin(), initial_name.end(), ' ', '_');
                std::replace(initial_name.begin(), initial_name.end(), '/', '_');

                GrayScaleScatCfg cfg = compo.value().get<GrayScaleScatCfg>();
                image2::Image grayscale = satdump::make_scatterometer_grayscale(*rad_products, cfg);

                std::string name = products->instrument_name + "_grayscale_" + initial_name;
                image2::save_img(grayscale, product_path + "/" + name);
            }
        }

        if (instrument_viewer_settings.contains("grayscale_projs"))
        {
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_viewer_settings["grayscale_projs"].items())
            {
                std::string initial_name = compo.key();
                std::replace(initial_name.begin(), initial_name.end(), ' ', '_');
                std::replace(initial_name.begin(), initial_name.end(), '/', '_');

                GrayScaleScatCfg cfg = compo.value().get<GrayScaleScatCfg>();
                nlohmann::json proj_cfg;
                image2::Image grayscale = satdump::make_scatterometer_grayscale_projs(*rad_products, cfg, nullptr, &proj_cfg);

                grayscale.to_rgb();

                if (compo.value().contains("draw_map"))
                {
                    if (compo.value()["draw_map"].get<bool>())
                    {
                        auto proj_func = satdump::reprojection::setupProjectionFunction(grayscale.width(), grayscale.height(), proj_cfg, {});
                        logger->info("Drawing map");
                        unsigned short color[4] = {0, 65535, 0};
                        //  map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                        //                                 grayscale,
                        //                                 color,
                        //                                 proj_func); // TODOIMG
                    }
                }

                std::string name = products->instrument_name + "_grayscale_proj_" + initial_name;
                image2::save_img(grayscale, product_path + "/" + name);
            }
        }
    }
}