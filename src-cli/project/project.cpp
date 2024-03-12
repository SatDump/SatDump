#include "logger.h"
#include "init.h"
#include "project.h"
#include "common/cli_utils.h"

#include "common/image/image.h"
#include "common/image/image_meta.h"
#include "products/image_products.h"

#include "common/projection/reprojector.h"
#include "resources.h"
#include "common/image/image_utils.h"

struct ProjectionLayer
{
    std::string name;
    image::Image<uint16_t> img;
    float opacity = 100;
    bool enabled = true;
    float progress = 0;
    bool old_algo = false;
};

int main_project(int argc, char *argv[])
{
    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_ERROR);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    logger->info("Starting projection tool...");

    std::vector<ProjectionLayer> projection_layers;
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

                //  logger->error(end - i);
                auto params = parse_common_flags(end - i, argv + i + 1);
                //     logger->trace("\n" + params.dump(4));

                if (flag == "layer")
                {
                    ProjectionLayer newlayer;
                    if (params["type"] == "product")
                    {
                        logger->info("Loading product...");
                        std::shared_ptr<satdump::Products> products_raw = satdump::loadProducts(params["file"]);
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

                    if (params.contains("opacity"))
                        newlayer.opacity = params["opacity"];
                    if (params.contains("old_algo"))
                        newlayer.opacity = params["old_algo"];

                    projection_layers.push_back(newlayer);
                }
                else if (flag == "target")
                {
                    target_cfg = params;
                    projections_image_width = params["width"];
                    projections_image_width = params["height"];
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

    // Generate all layers
    std::vector<image::Image<uint16_t>> layers_images;

    for (int i = projection_layers.size() - 1; i >= 0; i--)
    {
        ProjectionLayer &layer = projection_layers[i];
        if (!layer.enabled)
            continue;
        //     if (progress_pointer == nullptr)
        //         progress_pointer = &layer.progress;

        int width = projections_image_width;
        int height = projections_image_height;

        satdump::reprojection::ReprojectionOperation op;

        if (!image::has_metadata_proj_cfg(layer.img)) // Just in case...
            continue;
        if (!image::get_metadata_proj_cfg(layer.img).contains("type")) // Just in case...
            continue;

        op.target_prj_info = target_cfg;
        op.img = layer.img;
        op.output_width = width;
        op.output_height = height;

        op.use_old_algorithm = layer.old_algo;

        image::Image<uint16_t> res = satdump::reprojection::reproject(op); //, progress_pointer);
        layers_images.push_back(res);

        // general_progress++;
    }

    ////////////////////////////////////////////////////////////////////

    // Setup final image
    image::Image<uint16_t> projected_image_result;
    projected_image_result.init(projections_image_width, projections_image_height, 3);
    projected_image_result.init_font(resources::getResourcePath("fonts/font.ttf"));

    logger->info("Combining images...");
    if (false) // Blend
    {
        projected_image_result = layers_images[0];
        for (int i = 1; i < (int)layers_images.size(); i++)
            projected_image_result = image::blend_images(projected_image_result, layers_images[i]);
    }
    else if (true)
    {
        projected_image_result = layers_images[0];
        for (int i = 1; i < (int)layers_images.size(); i++)
        {
            projected_image_result = image::merge_images_opacity(projected_image_result,
                                                                 layers_images[i],
                                                                 projection_layers[(projection_layers.size() - 1) - i].opacity / 100.0f);
        }
    }
    else
    {
        // This sucks but temporary
        for (auto &img : layers_images)
            projected_image_result.draw_image(0, img);
    }

    ////////////////////////////////////////////////////////////////////

    projected_image_result.save_img(target_cfg["file"]);

    return 0;
}
