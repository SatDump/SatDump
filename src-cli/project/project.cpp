#include "logger.h"
#include "init.h"
#include "project.h"
#include "common/cli_utils.h"

#include "common/image/image.h"

#include "common/projection/reprojector.h"
#include "resources.h"
#include "common/image/image_utils.h"
#include "common/overlay_handler.h"

#include "common/projection/reprojector_backend_utils.h"

bool isFlag(std::string flag)
{
    if (flag[0] == '-' && flag[1] != '-')
    {
        flag = flag.substr(1, flag.length());
        if (flag == "layer" || flag == "target")
            return true;
    }
    return false;
}

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

            if (isFlag(flag)) // Detect a flag
            {
                flag = flag.substr(1, flag.length()); // Remove the "--"

                //    logger->trace(flag);

                int end = 0;
                for (int ii = i + 1; ii < argc; ii++)
                {
                    end = ii;
                    std::string flag2 = argv[ii];
                    if (isFlag(flag2)) // Detect a flag
                        break;
                }

                //    for (int xi = 0; xi < end; xi++)
                //        logger->trace((argv + i + 1)[xi]);
                //  logger->error(end - i);
                auto params = parse_common_flags(end - i, argv + i + 1);
                //    logger->trace("\n" + params.dump(4));

                if (flag == "layer")
                {
                    logger->info("Loading layer...");
                    satdump::ProjectionLayer newlayer = satdump::loadExternalLayer((satdump::LayerLoadingConfig)params);

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
                }
                else
                {
                    throw satdump_exception("Invalid tag. Must be -layer or -target!");
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
