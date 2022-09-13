#include "viewer.h"
#include "error.h"
#include "common/map/map_drawer.h"
#include "common/projection/reprojector.h"
#include "logger.h"
#include "resources.h"
#include "core/style.h"
#include "main_ui.h"
#include "common/image/image_utils.h"
#include "common/widgets/switch.h"
namespace satdump
{
    void ViewerApplication::drawProjectionPanel()
    {
        if (ImGui::CollapsingHeader("Projection"))
        {
            ImGui::Combo("##targetproj", &projections_current_selected_proj, "Equirectangular\0"
                                                                             "Mercator\0"
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
            }
            else if (projections_current_selected_proj == 2)
            {
                ImGui::Text("Center :");
                ImGui::InputFloat("Lon##stereo", &projections_stereo_center_lon);
                ImGui::InputFloat("Lat##stereo", &projections_stereo_center_lat);
                ImGui::InputFloat("Scale##stereo", &projections_stereo_scale);
            }
            else if (projections_current_selected_proj == 3)
            {
                ImGui::Text("Center :");
                ImGui::InputFloat("Lon##tpers", &projections_tpers_lon);
                ImGui::InputFloat("Lat##tpers", &projections_tpers_lat);
                ImGui::InputFloat("Alt (kM)##tpers", &projections_tpers_alt);
                ImGui::InputFloat("Angle##tpers", &projections_tpers_ang);
                ImGui::InputFloat("Azimuth##tpers", &projections_tpers_azi);
            }

            if (ImGui::Button("Save"))
            {
                std::string default_name = "projection.png";

#ifndef __ANDROID__
                auto result = pfd::save_file("Save Projection", default_name, {"*.png"});
                while (!result.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (result.result().size() > 0)
                {
                    std::string path = result.result();
                    logger->info("Saving current projection at {:s}", path.c_str());
                    projected_image_result.save_png(path);
                }
#else
                std::string path = "/storage/emulated/0/" + default_name;
                logger->info("Saving current projection at {:s}", path.c_str());
                projected_image_result.save_png("" + path);
#endif
            }
        }
        if (ImGui::CollapsingHeader("Layers"))
        {
            /*
            ImGui::Text("Mode :");
            ImGui::RadioButton("Blend", &projections_mode_radio, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Overlay", &projections_mode_radio, 1);
            */

            ImGui::BeginGroup();
            ImGui::Text("Mode :  ");
            ImGui::SameLine();
            ImGui::Text("Blend");
            ImGui::SameLine();

            ImGui::SetNextItemWidth(50);
            ToggleButton("##projtog", &projections_mode_radio);
            ImGui::SameLine();
            ImGui::Text("Overlay");
            ImGui::EndGroup();

            ImGui::Separator(); //////////////////////////////////////////////////////

            ImGui::RadioButton("Equirectangular", &selected_external_type, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Other", &selected_external_type, 1);

            ImGui::InputText("Name", &projection_new_layer_name);
            projection_new_layer_file.draw();
            if (selected_external_type == 1)
                projection_new_layer_cfg.draw();

            if (ImGui::Button("Add layer") && projection_new_layer_file.file_valid)
            {
                // TMP
                ExternalProjSource new_layer_cfg;
                new_layer_cfg.name = projection_new_layer_name;
                if (selected_external_type == 1)
                    new_layer_cfg.path = projection_new_layer_cfg.getPath();
                else
                    new_layer_cfg.cfg = nlohmann::json::parse("{\"type\":\"equirectangular\",\"tl_lon\":-180,\"tl_lat\":90,\"br_lon\":180,\"br_lat\":-90}");
                new_layer_cfg.img.load_png(projection_new_layer_file.getPath());
                projections_external_sources.push_back(new_layer_cfg);

                refreshProjectionLayers();
            }

            ImGui::Separator(); ///////////////////////////////////////////////////

            ImGui::Text("Layers :");
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth());

            if (ImGui::BeginListBox("##pipelineslistbox"))
            {
                for (int i = 0; i < (int)projection_layers.size(); i++)
                {
                    ImGui::BeginGroup();
                    ProjectionLayer &layer = projection_layers[i];
                    std::string label = layer.viewer_prods->products->instrument_name;
                    if (layer.viewer_prods->handler->instrument_cfg.contains("name"))
                        label = layer.viewer_prods->handler->instrument_cfg["name"].get<std::string>();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + 4 * ui_scale));
                    ImGui::Text(label.c_str());
                    if(ImGui::IsItemHovered()){
                        ImGui::SetTooltip(layer.viewer_prods->dataset_name.c_str());
                    }
                    ImGui::SameLine(ImGui::GetWindowWidth() - 40 * ui_scale);
                    ImGui::SetCursorPosY(ImGui::GetCursorPos().y - 2 * ui_scale);
                    ImGui::Checkbox(std::string("##enablelayer" + layer.name + std::to_string(i)).c_str(), &layer.enabled);
                    // ImGui::DragFloat(std::string("Opacity##opacitylayer" + layer.name + std::to_string(i)).c_str(), &layer.opacity, 1.0, 0, 100);
                    ImGui::Image((void *)(intptr_t)layer.getPreview(), {50 * ui_scale, 50 * ui_scale});
                    ImGui::SameLine();

                    ImGui::BeginGroup();
                    FancySlider(std::string("##opacitylayer" + layer.name + std::to_string(i)).c_str(), "Opacity", &layer.opacity, ImGui::GetWindowWidth() - 76 * ui_scale);
                    ImGui::ProgressBar(layer.progress, ImVec2(ImGui::GetWindowWidth() - 76 * ui_scale, ImGui::GetFrameHeight()));
                    ImGui::EndGroup();

                    if (layer.type == 1)
                    {
                        if (ImGui::Button("Delete"))
                        {
                            projection_layers.erase(projection_layers.begin() + i);
                            break;
                        }
                    }
                    ImGui::EndGroup();
                    ImGui::Separator();
                }
                ImGui::EndListBox();
            }

            if (projections_are_generating || projection_layers.size() == 0)
                style::beginDisabled();
            if (ImGui::Button("GENERATE"))
            {
                ui_thread_pool.push([this](int)
                                    { 
                    logger->info("Update projection...");
                    generateProjectionImage();
                    logger->info("Done"); });
            }
            if (projections_are_generating || projection_layers.size() == 0)
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
            cfg["type"] = "mercator";
            //  cfg["tl_lon"] = projections_equirectangular_tl_lon;
            //  cfg["tl_lat"] = projections_equirectangular_tl_lat;
            //  cfg["br_lon"] = projections_equirectangular_br_lon;
            //  cfg["br_lat"] = projections_equirectangular_br_lat;
        }
        else if (projections_current_selected_proj == 2)
        {
            cfg["type"] = "stereo";
            cfg["center_lon"] = projections_stereo_center_lon;
            cfg["center_lat"] = projections_stereo_center_lat;
            cfg["scale"] = projections_stereo_scale;
        }
        else if (projections_current_selected_proj == 3)
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
            if (!layer.enabled)
                continue;

            if (layer.type == 0)
            {
                layer.viewer_prods->handler->updateProjection(projections_image_width, projections_image_height, cfg, &layer.progress);
                layer.viewer_prods->handler->getProjection().to_rgb();
                layers_images.push_back(layer.viewer_prods->handler->getProjection());
            }
            else
            {
                auto img = projectExternal(projections_image_width, projections_image_height, cfg, *layer.external, &layer.progress);
                img.to_rgb();
                layers_images.push_back(img);
            }
        }

        logger->info("Combining images...");
        if (projections_mode_radio == 0) // Blend
        {
            projected_image_result = layers_images[0];
            for (int i = 1; i < (int)layers_images.size(); i++)
                projected_image_result = image::blend_images(projected_image_result, layers_images[i]);
        }
        else if (projections_mode_radio == 1)
        {
            projected_image_result = layers_images[0];
            for (int i = 1; i < (int)layers_images.size(); i++)
                projected_image_result = image::merge_images_opacity(projected_image_result,
                                                                     layers_images[i],
                                                                     projection_layers[i].opacity / 100.0f);
        }
        else
        {
            // This sucks but temporary
            for (auto &img : layers_images)
                projected_image_result.draw_image(0, img);
        }

        // Free up memory
        layers_images.clear();

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

    image::Image<uint16_t> ViewerApplication::projectExternal(int width, int height, nlohmann::json tcfg, ExternalProjSource &ep, float *progress)
    {
        reprojection::ReprojectionOperation op;
        if (ep.path.size() > 0)
            op.source_prj_info = loadJsonFile(ep.path);
        else
            op.source_prj_info = ep.cfg;
        op.target_prj_info = tcfg;
        op.img = ep.img;
        op.output_width = width;
        op.output_height = height;
        op.use_draw_algorithm = false;
        reprojection::ProjectionResult res = reprojection::reproject(op, progress);
        return res.img;
    }

    void ViewerApplication::refreshProjectionLayers()
    {
        projection_layers.clear();

        for (ExternalProjSource &prjext : projections_external_sources)
            projection_layers.push_back({prjext.name, 1, nullptr, &prjext});

        for (int i = 0; i < (int)products_and_handlers.size(); i++)
        {
            if (products_and_handlers[i].handler->canBeProjected() && products_and_handlers[i].handler->shouldProject())
            {
                std::string label = products_and_handlers[i].products->instrument_name;
                projection_layers.push_back({label, 0, &products_and_handlers[i], nullptr});
            }
        }
    }
}