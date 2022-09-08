#include "viewer.h"
#include "error.h"
#include "common/map/map_drawer.h"
#include "common/projection/reprojector.h"
#include "logger.h"
#include "resources.h"
#include "core/style.h"
#include "main_ui.h"

image::Image<uint16_t> blend_imgs(image::Image<uint16_t> img1, image::Image<uint16_t> img2)
{
    image::Image<uint16_t> img_b(img1.width(), img1.height(), img1.channels());
    for (int c = 0; c < img1.channels(); c++)
    {
        for (size_t i = 0; i < img1.height() * img1.width(); i++)
        {
            if (img1.channel(c)[i] == 0)
                img_b.channel(c)[i] = img2.channel(c)[i];
            else if (img2.channel(c)[i] == 0)
                img_b.channel(c)[i] = img1.channel(c)[i];
            else
                img_b.channel(c)[i] = (size_t(img1.channel(c)[i]) + size_t(img2.channel(c)[i])) / 2;
        }
    }
    return img_b;
}

image::Image<uint16_t> merge_op_imgs(image::Image<uint16_t> img1, image::Image<uint16_t> img2, float op1, float op2)
{
    image::Image<uint16_t> img_b(img1.width(), img1.height(), img1.channels());
    for (int c = 0; c < img1.channels(); c++)
        for (size_t i = 0; i < img1.height() * img1.width(); i++)
            img_b.channel(c)[i] = (size_t(img1.channel(c)[i] * op1) + size_t(img2.channel(c)[i] * op2)) / 2;
    return img_b;
}

namespace satdump
{
    void ViewerApplication::drawProjectionPanel()
    {
        if (ImGui::CollapsingHeader("Projection"))
        {
            ImGui::Combo("##targetproj", &projections_current_selected_proj, "Equirectangular\0"
                                                                             "Stereo\0"
                                                                             "Satellite\0");
            ImGui::InputInt("Image Width", &projections_image_width);
            ImGui::InputInt("Image Height", &projections_image_height);

            if (projections_current_selected_proj == 0)
            {
                ImGui::Text("Top Left :");
                ImGui::InputFloat("Lon##tl", &projections_equirectangular_tl_lon);
                ImGui::InputFloat("Lat##tl", &projections_equirectangular_tl_lat);
                ImGui::Text("Bottom Right :");
                ImGui::InputFloat("Lon##br", &projections_equirectangular_br_lon);
                ImGui::InputFloat("Lat##br", &projections_equirectangular_br_lat);
            }
            else if (projections_current_selected_proj == 1)
            {
                ImGui::Text("Center :");
                ImGui::InputFloat("Lon##stereo", &projections_stereo_center_lon);
                ImGui::InputFloat("Lat##stereo", &projections_stereo_center_lat);
                ImGui::InputFloat("Scale##stereo", &projections_stereo_scale);
            }
            else if (projections_current_selected_proj == 2)
            {
                ImGui::Text("Center :");
                ImGui::InputFloat("Lon##tpers", &projections_tpers_lon);
                ImGui::InputFloat("Lat##tpers", &projections_tpers_lat);
                ImGui::InputFloat("Alt (kM)##tpers", &projections_tpers_alt);
                ImGui::InputFloat("Angle##tpers", &projections_tpers_ang);
                ImGui::InputFloat("Azimuth##tpers", &projections_tpers_azi);
            }
        }
        if (ImGui::CollapsingHeader("Layers"))
        {
            // int itm_radio = 0;
            // ImGui::Text("Mode :");
            // ImGui::RadioButton("Blend", &itm_radio, 0);
            // ImGui::SameLine();
            // ImGui::RadioButton("Overlay", &itm_radio, 1);

            ImGui::Text("Layers :");

            bool select = false;
            if (ImGui::BeginListBox("##pipelineslistbox"))
            {
                for (ProjectionLayer &layer : projection_layers)
                {
                    ImGui::Selectable(layer.name.c_str(), &select);
                    ImGui::DragFloat(std::string("Opacity##opacitylayer" + layer.name).c_str(), &layer.opacity, 1.0, 0, 100);
                    ImGui::Checkbox(std::string("Show##enablelayer" + layer.name).c_str(), &layer.enabled);
                    ImGui::ProgressBar(layer.progress);
                }
                ImGui::EndListBox();
            }

            if (projections_are_generating)
                style::beginDisabled();
            if (ImGui::Button("GENERATE"))
            {
                ui_thread_pool.push([this](int)
                                    { 
                    logger->info("Update projection...");
                    generateProjectionImage();
                    logger->info("Done"); });
            }
            if (projections_are_generating)
                style::endDisabled();
        }
        if (ImGui::CollapsingHeader("Overlay##viewerpojoverlay"))
        {
            ImGui::Checkbox("Map Overlay##Projs", &projections_draw_map_overlay);
            ImGui::Checkbox("Cities Overlay##Projs", &projections_draw_cities_overlay);
            ImGui::InputFloat("Cities Scale##Projs", &projections_cities_scale);
        }
    }

    void ViewerApplication::generateProjectionImage()
    {
        projections_are_generating = true;

        nlohmann::json cfg; //= nlohmann::json::parse("{\"type\":\"equirectangular\",\"tl_lon\":-180,\"tl_lat\":90,\"br_lon\":180,\"br_lat\":-90}");

        if (projections_current_selected_proj == 0)
        {
            cfg["type"] = "equirectangular";
            cfg["tl_lon"] = projections_equirectangular_tl_lon;
            cfg["tl_lat"] = projections_equirectangular_tl_lat;
            cfg["br_lon"] = projections_equirectangular_br_lon;
            cfg["br_lat"] = projections_equirectangular_br_lat;
        }
        else if (projections_current_selected_proj == 1)
        {
            cfg["type"] = "stereo";
            cfg["center_lon"] = projections_stereo_center_lon;
            cfg["center_lat"] = projections_stereo_center_lat;
            cfg["scale"] = projections_stereo_scale;
        }
        else if (projections_current_selected_proj == 2)
        {
            cfg["type"] = "tpers";
            cfg["lon"] = projections_tpers_lon;
            cfg["lat"] = projections_tpers_lat;
            cfg["alt"] = projections_tpers_alt;
            cfg["ang"] = projections_tpers_ang;
            cfg["azi"] = projections_tpers_azi;
        }

        // Setup final image
        projected_image_result.init(projections_image_width, projections_image_height, 3);

        // Generate all layers
        std::vector<image::Image<uint16_t>> layers_images;
        for (ProjectionLayer &layer : projection_layers)
        {
            layer.viewer_prods->handler->updateProjection(projections_image_width, projections_image_height, cfg, &layer.progress);
            layers_images.push_back(layer.viewer_prods->handler->getProjection());
        }

        // This sucks but temporary
        for (auto &img : layers_images)
            projected_image_result.draw_image(0, img);

        // Setup projection to draw stuff on top
        auto proj_func = satdump::reprojection::setupProjectionFunction(projections_image_width, projections_image_height, cfg);

        // Draw map borders
        if (projections_draw_map_overlay)
        {
            logger->info("Drawing map overlay...");
            unsigned short color[3] = {0, 65535, 0};
            map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                           projected_image_result,
                                           color,
                                           proj_func);
        }

        // Draw cities points
        if (projections_draw_cities_overlay)
        {
            logger->info("Drawing map overlay...");
            unsigned short color[3] = {65535, 0, 0};
            map::drawProjectedCapitalsGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")},
                                              projected_image_result,
                                              color,
                                              proj_func,
                                              projections_cities_scale);
        }

        // Update ImageView
        projection_image_widget.update(projected_image_result);

        projections_are_generating = false;
    }
}