#include "processor.h"

#include "logger.h"
#include "../dataset.h"
#include "../image_products.h"
#include "core/config.h"

#include "resources.h"
#include "common/projection/reprojector.h"
#include "common/map/map_drawer.h"
#include "common/utils.h"

namespace satdump
{
    reprojection::ProjectionResult projectImg(nlohmann::json proj_settings, nlohmann::json metadata, image::Image<uint16_t> &img, std::vector<double> timestamps, ImageProducts &img_products)
    {
        reprojection::ReprojectionOperation op;
        op.source_prj_info = img_products.get_proj_cfg();
        op.target_prj_info = proj_settings["config"];
        if (!op.target_prj_info.contains("tl_lon"))
            op.target_prj_info["tl_lon"] = -180;
        if (!op.target_prj_info.contains("tl_lat"))
            op.target_prj_info["tl_lat"] = 90;
        if (!op.target_prj_info.contains("br_lon"))
            op.target_prj_info["br_lon"] = 180;
        if (!op.target_prj_info.contains("br_lat"))
            op.target_prj_info["br_lat"] = -90;
        op.img = img;
        op.output_width = proj_settings["width"].get<int>();
        op.output_height = proj_settings["height"].get<int>();
        op.img_tle = img_products.get_tle();
        op.img_tim = timestamps;
        if (proj_settings.contains("old_algo"))
            op.use_draw_algorithm = proj_settings["old_algo"];

        if (proj_settings.contains("equalize"))
            if (proj_settings["equalize"].get<bool>())
                op.img.equalize();

        reprojection::ProjectionResult ret = reprojection::reproject(op);

        if (proj_settings.contains("draw_map"))
        {
            if (proj_settings["draw_map"].get<bool>())
            {
                auto proj_func = satdump::reprojection::setupProjectionFunction(ret.img.width(), ret.img.height(), ret.settings, metadata);
                logger->info("Drawing map");
                unsigned short color[3] = {0, 65535, 0};
                map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                               ret.img,
                                               color,
                                               proj_func);
            }
        }

        ret.img.to_rgb();

        return ret;
    }

    void process_image_products(Products *products, std::string product_path)
    {
        ImageProducts *img_products = (ImageProducts *)products;

        // Get instrument settings
        nlohmann::ordered_json instrument_viewer_settings;
        if (config::main_cfg["viewer"]["instruments"].contains(products->instrument_name))
            instrument_viewer_settings = config::main_cfg["viewer"]["instruments"][products->instrument_name];
        else
            logger->error("Unknown instrument : %s!", products->instrument_name.c_str());

        // Generate composites
        if (instrument_viewer_settings.contains("rgb_composites"))
        {
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_viewer_settings["rgb_composites"].items())
            {
                try
                {
                    if (compo.value().contains("autogen")) // Skip auto-generating if requested
                        if (compo.value()["autogen"].get<bool>() == false)
                            continue;

                    // rgb_presets.push_back({compo.key(), compo.value().get<ImageCompositeCfg>()});
                    std::string initial_name = compo.key();
                    std::replace(initial_name.begin(), initial_name.end(), ' ', '_');
                    std::replace(initial_name.begin(), initial_name.end(), '/', '_');

                    ImageCompositeCfg cfg = compo.value().get<ImageCompositeCfg>();
                    std::vector<double> final_timestamps;
                    nlohmann::json final_metadata;
                    image::Image<uint16_t> rgb_image = satdump::make_composite_from_product(*img_products, cfg, nullptr, &final_timestamps, &final_metadata);

                    std::string name = products->instrument_name +
                                       (rgb_image.channels() == 1 ? "_" : "_rgb_") +
                                       initial_name;

                    if (compo.value().contains("map_overlay") ? compo.value()["map_overlay"].get<bool>() : false)
                    {
                        rgb_image.to_rgb(); // Ensure this is RGB!!
                        auto proj_func = satdump::reprojection::setupProjectionFunction(rgb_image.width(),
                                                                                        rgb_image.height(),
                                                                                        img_products->get_proj_cfg(),
                                                                                        final_metadata,
                                                                                        img_products->get_tle(),
                                                                                        final_timestamps);
                        logger->info("Drawing map...");
                        unsigned short color[3] = {0, 65535, 0};

                        if (compo.value().contains("map_overlay_colors"))
                        {
                            color[0] = compo.value()["map_overlay_colors"].get<std::vector<float>>()[0] * 65535;
                            color[1] = compo.value()["map_overlay_colors"].get<std::vector<float>>()[1] * 65535;
                            color[2] = compo.value()["map_overlay_colors"].get<std::vector<float>>()[2] * 65535;
                        }

                        map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                                       rgb_image,
                                                       color,
                                                       proj_func,
                                                       100);
                    }

                    rgb_image.save_img(product_path + "/" + name);

                    if (compo.value().contains("project") && img_products->has_proj_cfg())
                    {
                        logger->debug("Reprojecting composite %s", name.c_str());
                        reprojection::ProjectionResult ret = projectImg(compo.value()["project"],
                                                                        final_metadata,
                                                                        rgb_image,
                                                                        final_timestamps,
                                                                        *img_products);
                        ret.img.save_img(product_path + "/rgb_" + name + "_projected");
                    }

                    if ((compo.value().contains("geo_correct") ? compo.value()["geo_correct"].get<bool>() : false) && rgb_image.size() > 0)
                    {
                        bool success = false;
                        rgb_image = perform_geometric_correction(*img_products, rgb_image, success);

                        if (success)
                        {
                            rgb_image.save_img(product_path + "/" + name + "_corrected");
                        }
                    }
                }
                catch (std::exception &e)
                {
                    logger->error("Error making composites : %s!", e.what());
                }
            }
        }

        // Single-channel projections
        if (instrument_viewer_settings.contains("project_channels") && img_products->has_proj_cfg())
        {
            std::vector<int> ch_to_prj;
            if (instrument_viewer_settings["project_channels"]["channels"] == "all")
            {
                for (int i = 0; i < (int)img_products->images.size(); i++)
                    ch_to_prj.push_back(i);
            }
            else
            {
                auto chs_str = splitString(instrument_viewer_settings["project_channels"]["channels"].get<std::string>(), ',');
                for (int i = 0; i < (int)img_products->images.size(); i++)
                {
                    bool has_ch = false;
                    for (std::string str : chs_str)
                    {
                        if (img_products->images[i].channel_name == str)
                            has_ch = true;
                    }
                    if (has_ch)
                        ch_to_prj.push_back(i);
                }
            }

            for (int chanid : ch_to_prj)
            {
                auto &img = img_products->images[chanid];

                logger->debug("Reprojecting channel %s", img.channel_name.c_str());
                reprojection::ProjectionResult ret = projectImg(instrument_viewer_settings["project_channels"],
                                                                img_products->get_channel_proj_metdata(chanid),
                                                                img.image,
                                                                img_products->get_timestamps(chanid),
                                                                *img_products);
                ret.img.save_img(product_path + "/channel_" + img.channel_name + "_projected");
            }
        }
    }
}