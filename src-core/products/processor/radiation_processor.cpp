#include "processor.h"

#include "logger.h"
#include "../radiation_products.h"
#include "core/config.h"
#include "common/image2/io.h"

namespace satdump
{
    void process_radiation_products(Products *products, std::string product_path)
    {
        RadiationProducts *rad_products = (RadiationProducts *)products;

        // Get instrument settings
        nlohmann::ordered_json instrument_viewer_settings;
        if (config::main_cfg["viewer"]["instruments"].contains(products->instrument_name))
            instrument_viewer_settings = config::main_cfg["viewer"]["instruments"][products->instrument_name];
        else
            logger->error("Unknown instrument : %s!", products->instrument_name.c_str());

        // TMP
        if (instrument_viewer_settings.contains("radiation_maps"))
        {
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_viewer_settings["radiation_maps"].items())
            {
                // rgb_presets.push_back({compo.key(), compo.value().get<ImageCompositeCfg>()});
                std::string initial_name = compo.key();
                std::replace(initial_name.begin(), initial_name.end(), ' ', '_');
                std::replace(initial_name.begin(), initial_name.end(), '/', '_');

                RadiationMapCfg cfg = compo.value().get<RadiationMapCfg>();
                image2::Image rad_map = satdump::make_radiation_map(*rad_products, cfg);

                std::string name = products->instrument_name + "_map_" + initial_name;

                image2::save_img(rad_map, product_path + "/" + name);
            }
        }
    }
}