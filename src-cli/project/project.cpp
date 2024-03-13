#include "logger.h"
#include "init.h"
#include "project.h"
#include "common/cli_utils.h"

#include "common/image/image.h"
#include "common/image/image_meta.h"
#include "products/image_products.h"
#include "products/scatterometer_products.h"
#include "products/radiation_products.h"

#include "common/projection/reprojector.h"
#include "resources.h"
#include "common/image/image_utils.h"
#include "common/overlay_handler.h"

#include "common/projection/reprojector_backend_utils.h"

int main_project(int argc, char *argv[])
{
    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_ERROR);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    logger->info("Starting projection tool...");

    std::vector<satdump::ProjectionLayer> projection_layers;
    nlohmann::json target_cfg;
    int projections_image_width = 0, projections_image_height = 0;

    for (int i = 0; i < argc; i++)
    {
        // logger->debug(argv[i]);

        if (i != argc)
        {
            std::string flag = argv[i];

            if (flag[0] == '-' && flag[1] != '-') // Detect a flag
            {
                flag = flag.substr(1, flag.length()); // Remove the "--"

                //    logger->trace(flag);

                int end = 0;
                for (int ii = i + 1; ii < argc; ii++)
                {
                    end = ii;
                    std::string flag2 = argv[ii];
                    if (flag2[0] == '-' && flag2[1] != '-') // Detect a flag
                        break;
                }

                //    for (int xi = 0; xi < end; xi++)
                //        logger->trace((argv + i + 1)[xi]);
                //  logger->error(end - i);
                auto params = parse_common_flags(end - i, argv + i + 1);
                //    logger->trace("\n" + params.dump(4));

                if (flag == "layer")
                {
                    satdump::ProjectionLayer newlayer;
                    if (params["type"] == "product")
                    {
                        logger->info("Loading product...");
                        std::shared_ptr<satdump::Products> products_raw = satdump::loadProducts(params["file"]);
                        if (products_raw->type == "image")
                        {
                            satdump::ImageProducts *products = (satdump::ImageProducts *)products_raw.get();

                            satdump::ImageCompositeCfg composite_config = params;
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

                            satdump::GrayScaleScatCfg cfg = params;
                            nlohmann::json proj_prm;
                            newlayer.img = make_scatterometer_grayscale_projs(*products, cfg, nullptr, &proj_prm);
                            image::set_metadata_proj_cfg(newlayer.img, proj_prm);
                        }
                        else if (products_raw->type == "radiation")
                        {
                            satdump::RadiationProducts *products = (satdump::RadiationProducts *)products_raw.get();

                            satdump::RadiationMapCfg cfg = params;
                            nlohmann::json proj_prm;
                            newlayer.img = make_radiation_map(*products, cfg, true);
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
                    else if (params["type"] == "equirectangular" || params["type"] == "other")
                    {
                        newlayer.img.load_img(params["file"]);
                        if (newlayer.img.size() > 0)
                        {
                            double tl_lon = -180;
                            double tl_lat = 90;
                            double br_lon = 180;
                            double br_lat = -90;
                            nlohmann::json proj_cfg;
                            if (params["type"] == "equirectangular")
                            {
                                proj_cfg["type"] = "equirec";
                                proj_cfg["offset_x"] = tl_lon;
                                proj_cfg["offset_y"] = tl_lat;
                                proj_cfg["scalar_x"] = (br_lon - tl_lon) / double(newlayer.img.width());
                                proj_cfg["scalar_y"] = (br_lat - tl_lat) / double(newlayer.img.height());
                            }
                            else if (params["type"] == "other")
                            {
                                proj_cfg = loadJsonFile(params["proj_cfg"]);
                            }
                            if (target_cfg.contains("normalize") ? target_cfg["normalize"].get<bool>() : false)
                                newlayer.img.normalize();
                            image::set_metadata_proj_cfg(newlayer.img, proj_cfg);
                        }
                        else
                            throw std::runtime_error("Could not load image file!");
                    }
                    else if (params["type"] == "geotiff")
                    {
                        newlayer.img.load_tiff(params["file"]);
                        if (newlayer.img.size() > 0 && image::has_metadata_proj_cfg(newlayer.img))
                        {
                            if (target_cfg.contains("normalize") ? target_cfg["normalize"].get<bool>() : false)
                                newlayer.img.normalize();
                        }
                        else
                            throw std::runtime_error("Could not load GeoTIFF. This may not be a TIFF file, or the projection settings are unsupported? If you think they should be supported, open an issue on GitHub.");
                    }

                    if (params.contains("opacity"))
                        newlayer.opacity = params["opacity"];
                    if (params.contains("old_algo"))
                        newlayer.opacity = params["old_algo"];

                    projection_layers.push_back(newlayer);
                }
                else if (flag == "target")
                {
                    target_cfg = params;
                    if (params.contains("width"))
                        projections_image_width = params["width"];
                    if (params.contains("height"))
                        projections_image_height = params["height"];

                    if (target_cfg.contains("scalar_y"))
                        target_cfg["scalar_y"] = -target_cfg["scalar_y"].get<double>();
                }
                else
                {
                    logger->error("Invalid tag. Must be -layer or -target!");
                    return 1;
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////

    bool projection_auto_mode = target_cfg.contains("auto_mode") ? target_cfg["auto_mode"].get<bool>() : false;
    bool projection_auto_scale_mode = target_cfg.contains("auto_scale_mode") ? target_cfg["auto_scale_mode"].get<bool>() : false;

    satdump::applyAutomaticProjectionSettings(projection_layers, projection_auto_mode, projection_auto_scale_mode, projections_image_width, projections_image_height, target_cfg);

    ////////////////////////////////////////////////////////////////////

    // Generate all layers
    std::vector<image::Image<uint16_t>> layers_images =
        satdump::generateAllProjectionLayers(projection_layers, projections_image_width, projections_image_height, target_cfg);

    ////////////////////////////////////////////////////////////////////

    // Setup final image
    image::Image<uint16_t> projected_image_result;
    projected_image_result.init(projections_image_width, projections_image_height, 3);
    projected_image_result.init_font(resources::getResourcePath("fonts/font.ttf"));

    logger->info("Combining images...");
    if (target_cfg.contains("blend_mode") ? target_cfg["blend_mode"].get<bool>() : false) // Blend
    {
        projected_image_result = layers_images[0];
        for (int i = 1; i < (int)layers_images.size(); i++)
            projected_image_result = image::blend_images(projected_image_result, layers_images[i]);
    }
    else
    {
        projected_image_result = layers_images[0];
        for (int i = 1; i < (int)layers_images.size(); i++)
        {
            projected_image_result = image::merge_images_opacity(projected_image_result,
                                                                 layers_images[i],
                                                                 projection_layers[(projection_layers.size() - 1) - i].opacity / 100.0f);
        }
    }

    ////////////////////////////////////////////////////////////////////

    OverlayHandler projection_overlay_handler;
    projection_overlay_handler.set_config(target_cfg);
    if (projection_overlay_handler.enabled())
    {
        auto proj_func = satdump::reprojection::setupProjectionFunction(projections_image_width, projections_image_height, target_cfg, {});
        projection_overlay_handler.clear_cache();
        projection_overlay_handler.apply(projected_image_result, proj_func, nullptr);
    }

    ////////////////////////////////////////////////////////////////////

    projected_image_result.save_img(target_cfg["file"]);

    return 0;
}
