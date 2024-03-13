#pragma once

#include "reprojector.h"
#include "imgui/imgui_image.h"
#include "common/image/image_meta.h"

namespace satdump
{
    struct ProjectionLayer
    {
        std::string name;
        image::Image<uint16_t> img;
        float opacity = 100;
        bool enabled = true;
        float progress = 0;
        bool old_algo = false;

        unsigned int preview_texid = 0;
        unsigned int getPreview()
        {
            if (preview_texid == 0)
            {
                preview_texid = makeImageTexture();
                auto img8 = img.resize_to(100, 100).to8bits();
                uint32_t *tmp_rgba = new uint32_t[img8.width() * img8.height()];
                uchar_to_rgba(img8.data(), tmp_rgba, img8.width() * img8.height(), img8.channels());
                updateImageTexture(preview_texid, tmp_rgba, img8.width(), img8.height());
                delete[] tmp_rgba;
            }
            return preview_texid;
        }
    };

    inline void applyAutomaticProjectionSettings(std::vector<ProjectionLayer> &projection_layers,
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

    inline std::vector<image::Image<uint16_t>> generateAllProjectionLayers(std::vector<ProjectionLayer> &projection_layers,
                                                                           int projections_image_width,
                                                                           int projections_image_height,
                                                                           nlohmann::json &target_cfg,
                                                                           float *general_progress = nullptr)
    {
        std::vector<image::Image<uint16_t>> layers_images;

        float *progress_pointer = nullptr;

        for (int i = projection_layers.size() - 1; i >= 0; i--)
        {
            ProjectionLayer &layer = projection_layers[i];
            if (!layer.enabled)
                continue;
            if (progress_pointer == nullptr)
                progress_pointer = &layer.progress;

            int width = projections_image_width;
            int height = projections_image_height;

            reprojection::ReprojectionOperation op;

            if (!image::has_metadata_proj_cfg(layer.img)) // Just in case...
                continue;
            if (!image::get_metadata_proj_cfg(layer.img).contains("type")) // Just in case...
                continue;

            op.target_prj_info = target_cfg;
            op.img = layer.img;
            op.output_width = width;
            op.output_height = height;

            op.use_old_algorithm = layer.old_algo;

            image::Image<uint16_t> res = reprojection::reproject(op, progress_pointer);
            layers_images.push_back(res);

            if (general_progress != nullptr)
                (*general_progress)++;
        }

        return layers_images;
    }
}