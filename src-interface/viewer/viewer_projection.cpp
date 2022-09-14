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
        if (ImGui::CollapsingHeader("Projection", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Output image : ");
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.40f);
            ImGui::InputInt("##width", &projections_image_width, 0);
            ImGui::SameLine();
            ImGui::Text(u8"\uea76");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.40f);
            ImGui::InputInt("##height", &projections_image_height, 0);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.96f);
            ImGui::Combo("##targetproj", &projections_current_selected_proj, "Equirectangular\0"
                                                                             "Mercator\0"
                                                                             "Stereo\0"
                                                                             "Satellite (TPERS)\0");

            if (projections_current_selected_proj == 0)
            {
                ImGui::Text("Top Left Coordinates :");
                ImGui::InputFloat("Lat##tl", &projections_equirectangular_tl_lat);
                ImGui::InputFloat("Lon##tl", &projections_equirectangular_tl_lon);
                ImGui::Spacing();
                ImGui::Text("Bottom Right Coordinates :");
                ImGui::InputFloat("Lat##br", &projections_equirectangular_br_lat);
                ImGui::InputFloat("Lon##br", &projections_equirectangular_br_lon);
            }
            else if (projections_current_selected_proj == 1)
            {
            }
            else if (projections_current_selected_proj == 2)
            {
                ImGui::Text("Center Coordinates :");
                ImGui::InputFloat("Lat##stereo", &projections_stereo_center_lat);
                ImGui::InputFloat("Lon##stereo", &projections_stereo_center_lon);
                ImGui::Spacing();
                ImGui::InputFloat("Scale##stereo", &projections_stereo_scale);
            }
            else if (projections_current_selected_proj == 3)
            {
                ImGui::Text("Center Coordinates :");
                ImGui::InputFloat("Lat##tpers", &projections_tpers_lat);
                ImGui::InputFloat("Lon##tpers", &projections_tpers_lon);
                ImGui::Spacing();
                ImGui::InputFloat("Altitude (km)##tpers", &projections_tpers_alt);
                ImGui::InputFloat("Angle##tpers", &projections_tpers_ang);
                ImGui::InputFloat("Azimuth##tpers", &projections_tpers_azi);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            bool beginGenHide = projections_are_generating;
            if (projections_are_generating || projection_layers.size() == 0)
                style::beginDisabled();
            if (ImGui::Button("Generate Projection"))
            {
                ui_thread_pool.push([this](int)
                                    { 
                    logger->info("Update projection...");
                    generateProjectionImage();
                    logger->info("Done"); });
            }
            if ((projections_are_generating || projection_layers.size() == 0) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("No layers loaded!");
            if ((projections_are_generating && beginGenHide) || projection_layers.size() == 0)
                style::endDisabled();

            ImGui::Spacing();

            if (ImGui::Button("Save Projected Image"))
            {
                std::string default_name = ".\\projection.png";

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

            ImGui::Text("Layers :");

            ImGui::SameLine(ImGui::GetWindowWidth() - 85 * ui_scale);

            if (is_opening_layer)
                style::beginDisabled();

            if (ImGui::Button("Add Layer##button"))
            {
                ImGui::OpenPopup("Add Layer##popup", ImGuiPopupFlags_None);
            }

            if (is_opening_layer)
                style::endDisabled();

            {
                if (ImGui::BeginPopupModal("Add Layer##popup", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::RadioButton("Equirectangular", &selected_external_type, 0);
                    if (already_has_osm_layer)
                        style::beginDisabled();
                    ImGui::RadioButton("Tile Map (OSM)", &selected_external_type, 2);
                    if (already_has_osm_layer)
                    {
                        ImGui::SetTooltip("Only one Tile Map layer can be loaded at a time!");
                        style::endDisabled();
                    }
                    ImGui::RadioButton("Other", &selected_external_type, 1);

                    if (selected_external_type == 2)
                    {
                        ImGui::SliderInt("Zoom##osmsliderzoom", &projection_osm_zoom, 0, 6);
                        ImGui::InputText("Tile URL", &mapurl, ImGuiInputTextFlags_None);
                    }
                    else
                    {
                        ImGui::InputText("Name", &projection_new_layer_name);
                        projection_new_layer_file.draw("Input Image");
                        if (selected_external_type == 1)
                            projection_new_layer_cfg.draw("Projection Config File");
                    }

                    if (ImGui::Button("Add layer") && (selected_external_type == 2 || projection_new_layer_file.file_valid))
                    {
                        auto genfun = [this](int)
                        {
                            is_opening_layer = true;

                            ExternalProjSource new_layer_cfg;
                            if (selected_external_type == 2)
                                new_layer_cfg.name = "Tile Map";
                            else
                                new_layer_cfg.name = projection_new_layer_name;

                            if (selected_external_type == 1)
                                new_layer_cfg.path = projection_new_layer_cfg.getPath();
                            else if (selected_external_type == 2)
                                new_layer_cfg.cfg = nlohmann::json::parse("{\"type\":\"mercator\"}");
                            else
                                new_layer_cfg.cfg = nlohmann::json::parse("{\"type\":\"equirectangular\",\"tl_lon\":-180,\"tl_lat\":90,\"br_lon\":180,\"br_lat\":-90}");

                            if (selected_external_type == 2)
                            {
                                logger->info("Generating tile map");
                                tileMap tile_map(mapurl);
                                new_layer_cfg.img = tile_map.getMapImage({-85.06, -180}, {85.06, 180}, projection_osm_zoom).to16bits();
                            }
                            else
                                new_layer_cfg.img.load_png(projection_new_layer_file.getPath());

                            projections_external_sources.push_back(new_layer_cfg);

                            if (selected_external_type == 2)
                            {
                                already_has_osm_layer = true;
                                selected_external_type = 0;
                            }

                            projections_should_refresh = true;
                            is_opening_layer = false;
                        };
                        ui_thread_pool.push(genfun);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();

                    if (ImGui::Button("Cancel"))
                        ImGui::CloseCurrentPopup();

                    ImGui::EndPopup();
                }
            }

            if (ImGui::BeginListBox("##pipelineslistbox", ImVec2(ImGui::GetWindowWidth(), 300 * ui_scale)))
            {
                for (int i = 0; i < (int)projection_layers.size(); i++)
                {
                    ImGui::PushID(i);
                    ImGui::BeginGroup();
                    ProjectionLayer &layer = projection_layers[i];
                    std::string label;
                    if (layer.type == 0)
                    {
                        label = layer.viewer_prods->products->instrument_name;
                        if (layer.viewer_prods->handler->instrument_cfg.contains("name"))
                            label = layer.viewer_prods->handler->instrument_cfg["name"].get<std::string>();
                    }
                    else
                        label = layer.name;
                    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + 4 * ui_scale));
                    ImGui::Text(label.c_str());
                    if (ImGui::IsItemHovered() && layer.type == 0)
                    {
                        ImGui::SetTooltip(layer.viewer_prods->dataset_name.c_str());
                    }
                    ImGui::SameLine(ImGui::GetWindowWidth() - 70 * ui_scale);
                    ImGui::SetCursorPosY(ImGui::GetCursorPos().y - 2 * ui_scale);
                    ImGui::Checkbox(std::string("##enablelayer" + layer.name + std::to_string(i)).c_str(), &layer.enabled);
                    // if (layer.type == 1)
                    {
                        // Closing button
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(ImGui::GetCursorPos().y - 2 * ui_scale);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 0, 0, 255));
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                        if (ImGui::Button(std::string(u8"\uf00d##layerdelete" + layer.name + std::to_string(i)).c_str()))
                        {
                            if (layer.type == 1 || layer.type == 2)
                            {
                                projection_layers.erase(projection_layers.begin() + i);
                                for (int f = 0; f < (int)projections_external_sources.size(); f++)
                                    if (projections_external_sources[f].name == projection_layers[i].name &&
                                        &projections_external_sources[f] == projection_layers[i].external)
                                        projections_external_sources.erase(projections_external_sources.begin() + f);
                            }
                            else if (layer.type == 0)
                            {
                                layer.viewer_prods->handler->setShouldProject(false);
                            }
                            refreshProjectionLayers();
                            ImGui::EndGroup();
                            ImGui::PopStyleColor();
                            ImGui::PopStyleColor();
                            ImGui::PopID();
                            break;
                        }
                        ImGui::PopStyleColor();
                        ImGui::PopStyleColor();
                    }
                    if (layer.enabled)
                    {
                        // ImGui::DragFloat(std::string("Opacity##opacitylayer" + layer.name + std::to_string(i)).c_str(), &layer.opacity, 1.0, 0, 100);
                        ImGui::Image((void *)(intptr_t)layer.getPreview(), {50 * ui_scale, 50 * ui_scale});
                        ImGui::SameLine();

                        ImGui::BeginGroup();
                        FancySlider(std::string("##opacitylayer" + layer.name + std::to_string(i)).c_str(), "Opacity", &layer.opacity, ImGui::GetWindowWidth() - 76 * ui_scale);
                        ImGui::ProgressBar(layer.progress, ImVec2(ImGui::GetWindowWidth() - 76 * ui_scale, ImGui::GetFrameHeight()));
                        ImGui::EndGroup();
                    }
                    ImGui::EndGroup();

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        // Set payload to carry the index of our item (could be anything)
                        ImGui::SetDragDropPayload("LAYER_PROJECTION", &i, sizeof(int));

                        ImGui::Text(label.c_str());
                        ImGui::EndDragDropSource();
                    }
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("LAYER_PROJECTION"))
                        {
                            IM_ASSERT(payload->DataSize == sizeof(int));
                            int payload_n = *(const int *)payload->Data;
                            ProjectionLayer pr = projection_layers[payload_n];
                            projection_layers.erase(projection_layers.begin() + payload_n);
                            auto it = projection_layers.begin();
                            projection_layers.insert(it + i, pr);
                        }
                        ImGui::EndDragDropTarget();
                    }
                    ImGui::PopID();
                    ImGui::Separator();
                }
                ImGui::EndListBox();
            }

            if (projections_are_generating || projection_layers.size() == 0)
                style::beginDisabled();
            if (ImGui::Button("Generate Projection##layers"))
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
            ImGui::SameLine();
            ImGui::ColorEdit3("##borders", (float *)&color_borders, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::Checkbox("Cities Overlay##Projs", &projections_draw_cities_overlay);
            ImGui::SameLine();
            ImGui::ColorEdit3("##cities", (float *)&color_cities, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::InputFloat("Cities Scale##Projs", &projections_cities_scale);
        }

        if (projections_should_refresh) // Refresh in the UI thread!
        {
            refreshProjectionLayers();
            projections_should_refresh = false;
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

        for (int i = projection_layers.size() - 1; i >= 0; i--)
        {
            ProjectionLayer &layer = projection_layers[i];
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
            unsigned short color[3] = {(unsigned short)(color_borders.x * 65535.0f), (unsigned short)(color_borders.y * 65535.0f), (unsigned short)(color_borders.z * 65535.0f)};
            map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                           projected_image_result,
                                           color,
                                           proj_func);
        }

        // Draw cities points
        if (projections_draw_cities_overlay)
        {
            logger->info("Drawing map overlay...");
            unsigned short color[3] = {(unsigned short)(color_cities.x * 65535.0f), (unsigned short)(color_cities.y * 65535.0f), (unsigned short)(color_cities.z * 65535.0f)};
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

        if (!op.source_prj_info.contains("type")) // Just in case...
            return image::Image<uint16_t>(width, height, 3);

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
        // Check for *new* ext layers
        for (int i = 0; i < (int)projections_external_sources.size(); i++)
        {
            bool contains = false;

            for (ProjectionLayer &lay : projection_layers)
            {
                if (projections_external_sources[i].name == lay.name &&
                    &projections_external_sources[i] == lay.external)
                    contains = true;

                if (projections_external_sources[i].name == "Tile Map" && lay.name == "Tile Map")
                {
                    already_has_osm_layer = true;
                    contains = true;
                }
            }

            if (!contains)
                projection_layers.push_back({projections_external_sources[i].name, 1, nullptr, &projections_external_sources[i]});
        }

        // Check for *new* products layers
        for (int i = 0; i < (int)products_and_handlers.size(); i++)
        {
            if (products_and_handlers[i].handler->canBeProjected() && products_and_handlers[i].handler->shouldProject())
            {
                std::string label = products_and_handlers[i].products->instrument_name;

                bool contains = false;
                for (ProjectionLayer &lay : projection_layers)
                {
                    if (lay.type != 0)
                        continue;

                    if (label == lay.name && &products_and_handlers[i] == lay.viewer_prods)
                        contains = true;
                }

                if (!contains)
                    projection_layers.push_back({label, 0, &products_and_handlers[i], nullptr});
            }
        }

    recheck:
        // Check for products layers to delete
        for (int i = 0; i < (int)projection_layers.size(); i++)
        {
            ProjectionLayer &lay = projection_layers[i];

            if (lay.type != 0)
                continue;

            bool contains = false;
            for (int i = 0; i < (int)products_and_handlers.size(); i++)
                if (products_and_handlers[i].handler->canBeProjected() && products_and_handlers[i].handler->shouldProject())
                    if (products_and_handlers[i].products->instrument_name == lay.name && &products_and_handlers[i] == lay.viewer_prods)
                        contains = true;

            if (!contains)
            {
                projection_layers.erase(projection_layers.begin() + i);
                goto recheck;
            }
        }
    }
}