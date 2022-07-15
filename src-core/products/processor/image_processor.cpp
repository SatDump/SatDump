#include "processor.h"

#include "logger.h"
#include "../dataset.h"
#include "../image_products.h"
#include "core/config.h"

namespace satdump
{
    void process_image_products(Products *products, std::string product_path)
    {
        ImageProducts *img_products = (ImageProducts *)products;

        // Get instrument settings
        nlohmann::ordered_json instrument_viewer_settings;
        if (config::main_cfg["viewer"]["instruments"].contains(products->instrument_name))
            instrument_viewer_settings = config::main_cfg["viewer"]["instruments"][products->instrument_name];
        else
            logger->error("Unknown instrument : {:s}!", products->instrument_name);

        // TMP
        if (instrument_viewer_settings.contains("rgb_composites"))
        {
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_viewer_settings["rgb_composites"].items())
            {
                // rgb_presets.push_back({compo.key(), compo.value().get<ImageCompositeCfg>()});
                std::string initial_name = compo.key();
                std::replace(initial_name.begin(), initial_name.end(), ' ', '_');
                std::replace(initial_name.begin(), initial_name.end(), '/', '_');

                ImageCompositeCfg cfg = compo.value().get<ImageCompositeCfg>();
                image::Image<uint16_t> rgb_image = satdump::make_composite_from_product(*img_products, cfg);

                std::string name = products->instrument_name +
                                   (rgb_image.channels() == 1 ? "_" : "_rgb_") +
                                   initial_name;

                logger->info("Saving " + product_path + "/" + name + ".png");
                rgb_image.save_png(product_path + "/" + name + ".png");

                bool geo_correct = compo.value().contains("geo_correct") ? compo.value()["geo_correct"].get<bool>() : false;

                if (geo_correct)
                {
                    bool success = false;
                    rgb_image = perform_geometric_correction(*img_products, rgb_image, success);

                    if (success)
                    {
                        logger->info("Saving " + product_path + "/" + name + "_corrected.png");
                        rgb_image.save_png(product_path + "/" + name + "_corrected.png");
                    }
                }
            }
        }
    }
}