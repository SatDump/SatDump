#include "processor.h"

#include "logger.h"
#include "../dataset.h"
#include "../image_products.h"
#include "core/config.h"
#include "resources.h"

#include "common/projection/reprojector.h"
#include "common/map/map_drawer.h"
#include "common/utils.h"
#include "common/overlay_handler.h"
#include "common/image/meta.h"
#include "common/image/processing.h"
#include "common/image/io.h"

#include <regex>

namespace satdump
{
    image::Image projectImg(nlohmann::json proj_settings, nlohmann::json metadata, image::Image &img, std::vector<double> timestamps, ImageProducts &img_products)
    {
        reprojection::ReprojectionOperation op;
        nlohmann::json proj_cfg;
        proj_cfg = img_products.get_proj_cfg();
        op.target_prj_info = proj_settings["config"];
        //        if (!op.target_prj_info.contains("tl_lon"))
        //            op.target_prj_info["tl_lon"] = -180;
        //        if (!op.target_prj_info.contains("tl_lat"))
        //            op.target_prj_info["tl_lat"] = 90;
        //        if (!op.target_prj_info.contains("br_lon"))
        //            op.target_prj_info["br_lon"] = 180;
        //        if (!op.target_prj_info.contains("br_lat"))
        //            op.target_prj_info["br_lat"] = -90;

        op.img = &img;
        proj_cfg["metadata"] = metadata;
        proj_cfg["metadata"]["tle"] = img_products.get_tle();
        proj_cfg["metadata"]["timestamps"] = timestamps;

        image::set_metadata_proj_cfg(*op.img, proj_cfg);

        if (op.target_prj_info.contains("auto") && op.target_prj_info["auto"].get<bool>())
        {
            auto bounds = reprojection::determineProjectionBounds(*op.img);
            logger->trace("Final Bounds are : %f, %f - %f, %f", bounds.min_lon, bounds.min_lat, bounds.max_lon, bounds.max_lat);
            reprojection::tryAutoTuneProjection(bounds, op.target_prj_info);
            logger->debug("%d, %d\n%s", op.output_width, op.output_height, op.target_prj_info.dump(4).c_str());
        }

        if (!op.target_prj_info.contains("width") || !op.target_prj_info.contains("height"))
        {
            logger->error("No width or height defined for projection!");
            return image::Image();
        }

        op.output_width = op.target_prj_info["width"].get<int>();
        op.output_height = op.target_prj_info["height"].get<int>();

        if (proj_settings.contains("old_algo"))
            op.use_old_algorithm = proj_settings["old_algo"];

        if (proj_settings.contains("equalize"))
            if (proj_settings["equalize"].get<bool>())
                image::equalize(*op.img);

        image::Image retimg = reprojection::reproject(op);

        OverlayHandler overlay_handler;
        overlay_handler.set_config(proj_settings);

        if (overlay_handler.enabled() && image::has_metadata_proj_cfg(retimg))
        {
            auto proj_func = satdump::reprojection::setupProjectionFunction(retimg.width(), retimg.height(), image::get_metadata_proj_cfg(retimg));
            overlay_handler.apply(retimg, proj_func);
        }

        retimg.to_rgba();

        return retimg;
    }

    void process_image_products(Products *products, std::string product_path)
    {
        ImageProducts *img_products = (ImageProducts *)products;

        // Overlay stuff
        OverlayHandler overlay_handler;
        OverlayHandler corrected_overlay_handler;
        std::function<std::pair<int, int>(double, double, double, double)> proj_func;
        std::function<std::pair<int, int>(double, double, double, double)> corr_proj_func;
        size_t last_width = 0, last_height = 0, last_corr_width = 0, last_corr_height = 0;
        nlohmann::json last_proj_cfg;

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
                    if (img_products->contents.contains("autocomposite_cache_enabled") && img_products->contents["autocomposite_cache_enabled"].get<bool>())
                        if (img_products->contents["autocomposite_cache_done"].contains(compo.key()))
                            continue;

                    // rgb_presets.push_back({compo.key(), compo.value().get<ImageCompositeCfg>()});
                    std::string initial_name = compo.key();
                    std::replace(initial_name.begin(), initial_name.end(), ' ', '_');
                    std::replace(initial_name.begin(), initial_name.end(), '/', '_');
                    initial_name = std::regex_replace(initial_name, std::regex(u8"\u00B5"), u8"u");
                    initial_name = std::regex_replace(initial_name, std::regex(u8"\u03BB="), u8"");
                    ImageCompositeCfg cfg = compo.value().get<ImageCompositeCfg>();
                    if (!check_composite_from_product_can_be_made(*img_products, cfg))
                    {
                        logger->debug("Skipping " + compo.key() + " as it can't be made!");
                        continue;
                    }

                    std::vector<double> final_timestamps;
                    nlohmann::json final_metadata;
                    image::Image rgb_image = satdump::make_composite_from_product(*img_products, cfg, nullptr, &final_timestamps, &final_metadata);

                    if (rgb_image.size() == 0)
                    {
                        logger->debug("Empty image, skipping any further processing!");
                        continue;
                    }

                    std::string name = products->instrument_name +
                                       (rgb_image.channels() == 1 ? "_" : "_rgb_") +
                                       initial_name;

                    bool geo_correct = compo.value().contains("geo_correct") && compo.value()["geo_correct"].get<bool>();
                    std::vector<float> corrected_stuff;
                    image::Image rgb_image_corr;

                    if (geo_correct)
                    {
                        corrected_stuff.resize(rgb_image.width());
                        bool success = false;
                        rgb_image_corr = perform_geometric_correction(*img_products, rgb_image, success, corrected_stuff.data());
                        if (!success)
                        {
                            geo_correct = false;
                            corrected_stuff.clear();
                        }

                        image::save_img(rgb_image_corr, product_path + "/" + name + "_corrected");
                    }

                    image::save_img(rgb_image, product_path + "/" + name);
                    overlay_handler.set_config(compo.value());
                    corrected_overlay_handler.set_config(compo.value());
                    if (overlay_handler.enabled())
                    {
                        // Ensure this is RGB!!
                        if (rgb_image.channels() < 3)
                            rgb_image.to_rgb();

                        nlohmann::json proj_cfg = img_products->get_proj_cfg();
                        proj_cfg["metadata"] = final_metadata;
                        proj_cfg["metadata"]["tle"] = img_products->get_tle();
                        proj_cfg["metadata"]["timestamps"] = final_timestamps;

                        if (last_width != rgb_image.width() || last_height != rgb_image.height() || last_proj_cfg != proj_cfg)
                        {
                            overlay_handler.clear_cache();

                            proj_func = satdump::reprojection::setupProjectionFunction(rgb_image.width(),
                                                                                       rgb_image.height(),
                                                                                       proj_cfg);

                            last_width = rgb_image.width();
                            last_height = rgb_image.height();
                            last_proj_cfg = proj_cfg;
                        }

                        if (geo_correct && (last_corr_width != rgb_image_corr.width() || last_corr_height != rgb_image_corr.height()))
                        {
                            corrected_overlay_handler.clear_cache();
                            corr_proj_func =
                                [&proj_func, corrected_stuff](double lat, double lon, int map_height, int map_width) mutable -> std::pair<int, int>
                            {
                                std::pair<int, int> ret = proj_func(lat, lon, map_height, map_width);
                                if (ret.first != -1 && ret.second != -1 && ret.first < (int)corrected_stuff.size() && ret.first >= 0)
                                {
                                    ret.first = corrected_stuff[ret.first];
                                }
                                else
                                    ret.second = ret.first = -1;
                                return ret;
                            };

                            last_corr_width = rgb_image_corr.width();
                            last_corr_height = rgb_image_corr.height();
                        }

                        overlay_handler.apply(rgb_image, proj_func);
                        image::save_img(rgb_image, product_path + "/" + name + "_map");
                        if (geo_correct)
                        {
                            corrected_overlay_handler.apply(rgb_image_corr, corr_proj_func);
                            image::save_img(rgb_image_corr, product_path + "/" + name + "_corrected_map");
                        }
                    }

                    // Free memory
                    if (geo_correct)
                        rgb_image_corr.clear();

                    if (compo.value().contains("project") && img_products->has_proj_cfg())
                    {
                        logger->debug("Reprojecting composite %s", name.c_str());
                        image::Image retimg = projectImg(compo.value()["project"],
                                                         final_metadata,
                                                         rgb_image,
                                                         final_timestamps,
                                                         *img_products);
                        std::string fmt = "";
                        if (compo.value()["project"].contains("img_format"))
                            fmt += compo.value()["project"]["img_format"].get<std::string>();
                        image::save_img(retimg, product_path + "/rgb_" + name + "_projected" + fmt);
                    }

                    if (img_products->contents.contains("autocomposite_cache_enabled") && img_products->contents["autocomposite_cache_enabled"].get<bool>())
                        img_products->contents["autocomposite_cache_done"][compo.key()] = true;
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
                try
                {
                    auto &img = img_products->images[chanid];

                    logger->debug("Reprojecting channel %s", img.channel_name.c_str());
                    image::Image retimg = projectImg(instrument_viewer_settings["project_channels"],
                                                     img_products->get_channel_proj_metdata(chanid),
                                                     img.image,
                                                     img_products->get_timestamps(chanid),
                                                     *img_products);
                    std::string fmt = "";
                    if (instrument_viewer_settings["project_channels"].contains("img_format"))
                        fmt += instrument_viewer_settings["project_channels"]["img_format"].get<std::string>();
                    image::save_img(retimg, product_path + "/channel_" + img.channel_name + "_projected" + fmt);
                }
                catch (std::exception &e)
                {
                    logger->error("Error projecting channel : %s!", e.what());
                }
            }
        }
    }
}