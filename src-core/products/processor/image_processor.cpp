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
        op.source_prj_info["metadata"] = metadata;
        op.source_prj_info["metadata"]["tle"] = img_products.get_tle();
        op.source_prj_info["metadata"]["timestamps"] = timestamps;

        if (proj_settings.contains("old_algo"))
            op.use_old_algorithm = proj_settings["old_algo"];

        if (proj_settings.contains("equalize"))
            if (proj_settings["equalize"].get<bool>())
                op.img.equalize();

        reprojection::ProjectionResult ret = reprojection::reproject(op);

        if (proj_settings.contains("draw_map"))
        {
            if (proj_settings["draw_map"].get<bool>())
            {
                auto proj_func = satdump::reprojection::setupProjectionFunction(ret.img.width(), ret.img.height(), ret.settings);
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
                    image::Image<uint16_t> rgb_image_corr;

                    std::string name = products->instrument_name +
                                       (rgb_image.channels() == 1 ? "_" : "_rgb_") +
                                       initial_name;

                    bool geo_correct = compo.value().contains("geo_correct") && compo.value()["geo_correct"].get<bool>();
                    bool map_overlay = compo.value().contains("map_overlay") && compo.value()["map_overlay"].get<bool>();
                    bool cities_overlay = compo.value().contains("cities_overlay");
                    bool latlon_overlay = compo.value().contains("latlon_grid") && compo.value()["latlon_grid"].get<bool>();
                    std::function<std::pair<int, int>(float, float, int, int)> proj_func;
                    std::function<std::pair<int, int>(float, float, int, int)> corr_proj_func;
                    std::vector<float> corrected_stuff;

                    if (geo_correct)
                    {
                        corrected_stuff.resize(rgb_image.width());
                        bool success = false;
                        image::Image<uint16_t> cor;
                        rgb_image_corr = perform_geometric_correction(*img_products, rgb_image, success, corrected_stuff.data());
                        if (!success)
                        {
                            geo_correct = false;
                            corrected_stuff.clear();
                        }
                    }

                    rgb_image.save_img(product_path + "/" + name);
                    if (geo_correct)
                        rgb_image_corr.save_img(product_path + "/" + name + "_corrected");

                    if (map_overlay || cities_overlay)
                    {
                        rgb_image.to_rgb(); // Ensure this is RGB!!
                        nlohmann::json proj_cfg = img_products->get_proj_cfg();
                        proj_cfg["metadata"] = final_metadata;
                        proj_cfg["metadata"]["tle"] = img_products->get_tle();
                        proj_cfg["metadata"]["timestamps"] = final_timestamps;
                        proj_func = satdump::reprojection::setupProjectionFunction(rgb_image.width(),
                                                                                   rgb_image.height(),
                                                                                   proj_cfg);

                        if (geo_correct)
                        {
                            std::function<std::pair<int, int>(float, float, int, int)> newfun =
                                [proj_func, corrected_stuff](float lat, float lon, int map_height, int map_width) mutable -> std::pair<int, int>
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
                            corr_proj_func = newfun;
                        }
                    }

                    if (map_overlay)
                    {
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
                        if (geo_correct)
                            map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                                           rgb_image_corr,
                                                           color,
                                                           corr_proj_func,
                                                           100);
                    }

                    if (latlon_overlay)
                    {
                        logger->info("Drawing latlon...");
                        unsigned short color[3] = {0, 0, 65535};

                        if (compo.value().contains("latlon_grid_colors"))
                        {
                            color[0] = compo.value()["latlon_grid_colors"].get<std::vector<float>>()[0] * 65535;
                            color[1] = compo.value()["latlon_grid_colors"].get<std::vector<float>>()[1] * 65535;
                            color[2] = compo.value()["latlon_grid_colors"].get<std::vector<float>>()[2] * 65535;
                        }

                        map::drawProjectedMapLatLonGrid(rgb_image,
                                                        color,
                                                        proj_func);
                        if (geo_correct)
                            map::drawProjectedMapLatLonGrid(
                                rgb_image_corr,
                                color,
                                corr_proj_func);
                    }

                    if (cities_overlay)
                    {
                        logger->info("Drawing cities...");
                        auto overlay_cfg = compo.value()["cities_overlay"].get<nlohmann::json>();
                        unsigned short color[3] = {65535, 0, 0};
                        int font_size = 50;
                        int cities_type = 0;
                        int scale_rank = 3;

                        if (overlay_cfg.contains("color"))
                        {
                            color[0] = overlay_cfg["color"].get<std::vector<float>>()[0] * 65535;
                            color[1] = overlay_cfg["color"].get<std::vector<float>>()[1] * 65535;
                            color[2] = overlay_cfg["color"].get<std::vector<float>>()[2] * 65535;
                        }
                        if (overlay_cfg.contains("font_size"))
                            font_size = overlay_cfg["font_size"].get<int>();
                        if (overlay_cfg.contains("type"))
                        {
                            if (overlay_cfg["type"] == "capitals")
                                cities_type = 0;
                            else if (overlay_cfg["type"] == "rcapitals")
                                cities_type = 1;
                            else if (overlay_cfg["type"] == "all")
                                cities_type = 2;
                        }
                        if (overlay_cfg.contains("scale_rank"))
                            scale_rank = overlay_cfg["scale_rank"].get<int>();

                        rgb_image.init_font(resources::getResourcePath("fonts/font.ttf"));
                        map::drawProjectedCitiesGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")},
                                                        rgb_image,
                                                        color,
                                                        proj_func,
                                                        font_size,
                                                        cities_type,
                                                        scale_rank);
                        if (geo_correct)
                        {
                            rgb_image_corr.init_font(resources::getResourcePath("fonts/font.ttf"));
                            map::drawProjectedCitiesGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")},
                                                            rgb_image_corr,
                                                            color,
                                                            corr_proj_func,
                                                            font_size,
                                                            cities_type,
                                                            scale_rank);
                        }
                    }

                    if (map_overlay || cities_overlay)
                    {
                        rgb_image.save_img(product_path + "/" + name + "_map");
                        if (geo_correct)
                            rgb_image_corr.save_img(product_path + "/" + name + "_corrected_map");
                    }

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