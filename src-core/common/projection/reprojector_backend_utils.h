#pragma once

#include <deque>
#include "core/exception.h"
#include "reprojector.h"
#include "imgui/imgui_image.h"
#include "common/image/meta.h"
#include "common/image/processing.h"
#include "common/image/io.h"

#include "common/utils.h"
#include "core/config.h"

#include "products/image_products.h"
#include "products/scatterometer_products.h"
#include "products/radiation_products.h"

namespace satdump
{
    struct ProjectionLayer
    {
        std::string name;
        image::Image img;
        float opacity = 100;
        bool enabled = true;
        float progress = 0;
        bool old_algo = false;
        bool allow_editor = false;

        unsigned int preview_texid = 0;
        unsigned int getPreview()
        {
            if (preview_texid == 0)
            {
                preview_texid = makeImageTexture();
                auto img8 = img.resize_to(100, 100).to8bits();
                uint32_t *tmp_rgba = new uint32_t[img8.width() * img8.height()];
                image::image_to_rgba(img8, tmp_rgba);
                updateImageTexture(preview_texid, tmp_rgba, img8.width(), img8.height());
                delete[] tmp_rgba;
            }
            return preview_texid;
        }
    };

    inline void applyAutomaticProjectionSettings(std::deque<ProjectionLayer> &projection_layers,
                                                 bool projection_auto_mode, bool projection_auto_scale_mode,
                                                 int &projections_image_width, int &projections_image_height,
                                                 nlohmann::json &target_cfg)
    {
        // Automatic projection settings!
        if (projection_auto_mode && (target_cfg["type"] == "equirec" || target_cfg["type"] == "stereo"))
        {
            satdump::reprojection::ProjBounds bounds;
            bounds.min_lon = 180;
            bounds.max_lon = -180;
            bounds.min_lat = 90;
            bounds.max_lat = -90;
            for (auto &layer : projection_layers)
            {
                if (!layer.enabled)
                    continue;
                auto boundshere = satdump::reprojection::determineProjectionBounds(layer.img);
                if (boundshere.valid)
                {
                    if (boundshere.min_lon < bounds.min_lon)
                        bounds.min_lon = boundshere.min_lon;
                    if (boundshere.max_lon > bounds.max_lon)
                        bounds.max_lon = boundshere.max_lon;
                    if (boundshere.min_lat < bounds.min_lat)
                        bounds.min_lat = boundshere.min_lat;
                    if (boundshere.max_lat > bounds.max_lat)
                        bounds.max_lat = boundshere.max_lat;
                }
            }

            logger->trace("Final Bounds are : %f, %f - %f, %f", bounds.min_lon, bounds.min_lat, bounds.max_lon, bounds.max_lat);

            if (projection_auto_scale_mode)
            {
                if (target_cfg.contains("width"))
                    target_cfg.erase("width");
                if (target_cfg.contains("height"))
                    target_cfg.erase("height");
            }
            else
            {
                if (target_cfg.contains("scale_x"))
                    target_cfg.erase("scale_x");
                if (target_cfg.contains("scale_y"))
                    target_cfg.erase("scale_y");
            }

            satdump::reprojection::tryAutoTuneProjection(bounds, target_cfg);
            if (target_cfg.contains("width"))
                projections_image_width = target_cfg["width"];
            if (target_cfg.contains("height"))
                projections_image_height = target_cfg["height"];

            logger->debug("\n%s", target_cfg.dump(4).c_str());
        }
    }

    inline std::vector<image::Image> generateAllProjectionLayers(std::deque<ProjectionLayer> &projection_layers,
                                                                 int projections_image_width,
                                                                 int projections_image_height,
                                                                 nlohmann::json &target_cfg,
                                                                 float *general_progress = nullptr)
    {
        std::vector<image::Image> layers_images;

        for (int i = projection_layers.size() - 1; i >= 0; i--)
        {
            logger->info("Reprojecting layer %d...", i);
            ProjectionLayer &layer = projection_layers[i];
            if (!layer.enabled)
                continue;
            if (layer.img.size() == 0)
            {
                logger->warn("Empty image! Skipping...");
                continue;
            }

            int width = projections_image_width;
            int height = projections_image_height;

            reprojection::ReprojectionOperation op;

            if (!image::has_metadata_proj_cfg(layer.img)) // Just in case...
                continue;
            if (!image::get_metadata_proj_cfg(layer.img).contains("type")) // Just in case...
                continue;

            op.target_prj_info = target_cfg;
            op.img = &layer.img;
            op.output_width = width;
            op.output_height = height;

            op.use_old_algorithm = layer.old_algo;

            image::Image res = reprojection::reproject(op, &layer.progress);
            layers_images.push_back(res);

            if (general_progress != nullptr)
                (*general_progress)++;
        }

        return layers_images;
    }

    struct LayerLoadingConfig
    {
        std::string type;
        std::string file;
        std::string projfile;
        bool normalize = false;
        nlohmann::json raw_cfg;
    };

    inline void from_json(const nlohmann::json &j, LayerLoadingConfig &v)
    {
        if (j.contains("type"))
            v.type = j["type"];
        else
            throw satdump_exception("Layer type must be present!");
        if (j.contains("file"))
            v.file = j["file"];
        if (j.contains("proj_cfg"))
            v.projfile = j["proj_cfg"];
        if (j.contains("normalize"))
            v.normalize = j["normalize"];
        v.raw_cfg = j;
    }

    inline ProjectionLayer loadExternalLayer(LayerLoadingConfig cfg)
    {
        satdump::ProjectionLayer newlayer;

        if (cfg.type == "product")
        {
            logger->info("Loading product...");
            std::shared_ptr<satdump::Products> products_raw = satdump::loadProducts(cfg.file);

            // Get instrument settings
            nlohmann::ordered_json instrument_cfg;
            if (satdump::config::main_cfg["viewer"]["instruments"].contains(products_raw->instrument_name))
                instrument_cfg = satdump::config::main_cfg["viewer"]["instruments"][products_raw->instrument_name];
            else
                logger->error("Unknown instrument : %s!", products_raw->instrument_name.c_str());

            if (products_raw->type == "image")
            {
                satdump::ImageProducts *products = (satdump::ImageProducts *)products_raw.get();

                satdump::ImageCompositeCfg composite_config;
                if (cfg.raw_cfg.contains("composite"))
                {
                    bool compo_set = false;
                    std::string search = cfg.raw_cfg["composite"];
                    if (instrument_cfg.contains("rgb_composites"))
                    {
                        for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_cfg["rgb_composites"].items())
                            if (check_composite_from_product_can_be_made(*products, compo.value().get<satdump::ImageCompositeCfg>()))
                                if (isStringPresent(compo.key(), search))
                                {
                                    logger->info("Composite candidate : " + compo.key());
                                    if (!compo_set)
                                    {
                                        composite_config = compo.value();
                                        compo_set = true;
                                    }
                                }
                    }
                }
                else
                {
                    composite_config = cfg.raw_cfg;
                }
                std::vector<double> final_timestamps;
                nlohmann::json final_metadata;
                newlayer.img = satdump::make_composite_from_product(*products, composite_config, nullptr, &final_timestamps, &final_metadata);
                nlohmann::json proj_cfg = products->get_proj_cfg();
                proj_cfg["metadata"] = final_metadata;
                if (products->has_tle())
                    proj_cfg["metadata"]["tle"] = products->get_tle();
                if (products->has_timestamps)
                    proj_cfg["metadata"]["timestamps"] = final_timestamps;
                image::set_metadata_proj_cfg(newlayer.img, proj_cfg);
            }
            else if (products_raw->type == "scatterometer")
            {
                satdump::ScatterometerProducts *products = (satdump::ScatterometerProducts *)products_raw.get();

                satdump::GrayScaleScatCfg _cfg = cfg.raw_cfg;
                nlohmann::json proj_prm;
                newlayer.img = make_scatterometer_grayscale_projs(*products, _cfg, nullptr, &proj_prm);
                image::set_metadata_proj_cfg(newlayer.img, proj_prm);
            }
            else if (products_raw->type == "radiation")
            {
                satdump::RadiationProducts *products = (satdump::RadiationProducts *)products_raw.get();

                satdump::RadiationMapCfg _cfg = cfg.raw_cfg;
                nlohmann::json proj_prm;
                newlayer.img = make_radiation_map(*products, _cfg, true);
                nlohmann::json proj_cfg;
                double tl_lon = -180;
                double tl_lat = 90;
                double br_lon = 180;
                double br_lat = -90;
                proj_cfg["type"] = "equirec";
                proj_cfg["offset_x"] = tl_lon;
                proj_cfg["offset_y"] = tl_lat;
                proj_cfg["scalar_x"] = (br_lon - tl_lon) / double(newlayer.img.width());
                proj_cfg["scalar_y"] = (br_lat - tl_lat) / double(newlayer.img.height());
                image::set_metadata_proj_cfg(newlayer.img, proj_cfg);
            }
        }
        else if (cfg.type == "equirectangular" || cfg.type == "other")
        {
            image::load_img(newlayer.img, cfg.file);
            if (newlayer.img.size() > 0)
            {
                double tl_lon = -180;
                double tl_lat = 90;
                double br_lon = 180;
                double br_lat = -90;
                nlohmann::json proj_cfg;
                if (cfg.type == "equirectangular")
                {
                    proj_cfg["type"] = "equirec";
                    proj_cfg["offset_x"] = tl_lon;
                    proj_cfg["offset_y"] = tl_lat;
                    proj_cfg["scalar_x"] = (br_lon - tl_lon) / double(newlayer.img.width());
                    proj_cfg["scalar_y"] = (br_lat - tl_lat) / double(newlayer.img.height());
                }
                else if (cfg.type == "other")
                {
                    proj_cfg = loadJsonFile(cfg.projfile);
                }
                if (cfg.normalize)
                    image::normalize(newlayer.img);
                image::set_metadata_proj_cfg(newlayer.img, proj_cfg);
            }
            else
                throw satdump_exception("Could not load image file!");
        }
        else if (cfg.type == "geotiff")
        {
            image::load_tiff(newlayer.img, cfg.file);
            if (newlayer.img.size() > 0 && image::has_metadata_proj_cfg(newlayer.img))
            {
                if (cfg.normalize)
                    image::normalize(newlayer.img);
            }
            else
                throw satdump_exception("Could not load GeoTIFF. This may not be a TIFF file, or the projection settings are unsupported? If you think they should be supported, open an issue on GitHub.");
        }
        else
            throw satdump_exception("Invalid Layer Type! " + cfg.type);

        return newlayer;
    }
}