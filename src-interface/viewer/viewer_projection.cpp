#include "viewer.h"
#include "common/map/map_drawer.h"
#include "common/projection/reprojector.h"
#include "logger.h"
#include "resources.h"
#include "core/style.h"
#include "main_ui.h"
#include "common/image/image_utils.h"
#include "common/widgets/switch.h"
#include "common/widgets/stepped_slider.h"
#include "libs/tiny-regex-c/re.h"
#include "imgui/pfd/pfd_utils.h"

namespace satdump
{
    re_t osm_url_regex = re_compile("https?:\\/\\/(www\\.)?[-a-zA-Z0-9@:%._\\+~#=]{2,256}\\.[a-z]{2,4}\\b([-a-zA-Z0-9@:%_\\+.~#?&//=]*)\\/\\{([xyz])\\}\\/\\{((?!\\3)[xyz])\\}\\/\\{((?!\\3)(?!\\4)[xyz])\\}(\\.png|\\.jpg|\\.jpeg|\\.j2k|)");
    int osm_url_regex_len = 0;
    float general_progress = 0;
    float general_sum = 1;
    float *progress_pointer = nullptr;

    void ViewerApplication::drawProjectionPanel()
    {
        bool disable_buttons = projections_are_generating;
        bool disable_add_layer = is_opening_layer;
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
                                                                             "Satellite (TPERS)\0"
                                                                             "Azimuthal Equidistant\0");

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
            else if (projections_current_selected_proj == 4)
            {
                ImGui::Text("Center Coordinates :");
                ImGui::InputFloat("Lat##eqaz", &projections_azeq_lat);
                ImGui::InputFloat("Lon##eqaz", &projections_azeq_lon);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (disable_buttons || projection_layers.size() == 0)
                style::beginDisabled();
            if (ImGui::Button("Generate Projection"))
            {
                ui_thread_pool.push([this](int)
                                    { 
                    logger->info("Update projection...");
                    generateProjectionImage();
                    logger->info("Done"); });
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                if (projections_are_generating)
                    ImGui::SetTooltip("Generating, please wait...");
                if (projection_layers.size() == 0)
                    ImGui::SetTooltip("No layers loaded!");
            }

            ImGui::Spacing();

            if (ImGui::Button("Save Projected Image"))
            {
                ui_thread_pool.push([this](int)
                                    {   projections_are_generating = true;
                        logger->info("Saving Projection...");
                        std::string default_path = config::main_cfg["satdump_directories"]["default_projection_output_directory"]["value"].get<std::string>();
                        std::string saved_at = save_image_dialog("projection", default_path, "Save Projection", &projected_image_result, &viewer_app->save_type);

                        if (saved_at == "")
                            logger->info("Save cancelled");
                        else
                            logger->info("Saved current projection at %s", saved_at.c_str());
                        projections_are_generating = false; });
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                if (projections_are_generating)
                    ImGui::SetTooltip("Generating, please wait...");
                if (projection_layers.size() == 0)
                    ImGui::SetTooltip("No layers loaded!");
            }
            if (disable_buttons || projection_layers.size() == 0)
                style::endDisabled();
        }
        if (ImGui::CollapsingHeader("Layers"))
        {
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
            if (disable_add_layer)
                style::beginDisabled();

            if (ImGui::Button("Add Layer##button"))
            {
                ImGui::OpenPopup("Add Layer##popup", ImGuiPopupFlags_None);
            }

            if (disable_add_layer)
                style::endDisabled();

            {
                if (ImGui::BeginPopupModal("Add Layer##popup", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    ImGui::RadioButton("Equirectangular", &selected_external_type, 0);
                    ImGui::RadioButton("Tile Map (OSM)", &selected_external_type, 2);
                    ImGui::RadioButton("Other", &selected_external_type, 1);

                    if (selected_external_type == 2)
                    {
                        ImGui::SliderInt("Zoom##osmsliderzoom", &projection_osm_zoom, 0, 6);
                        if (!urlgood)
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 0, 0, 255));

                        ImGui::InputText("Tile URL", &mapurl, ImGuiInputTextFlags_None);

                        if (!urlgood)
                        {
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("Invalid URL! must be https://server/{z}/{x}/{y}.ext");
                            ImGui::PopStyleColor();
                        }
                    }
                    else
                    {
                        ImGui::InputText("Name", &projection_new_layer_name);
                        projection_new_layer_file.draw("Input Image");
                        if (selected_external_type == 1)
                            projection_new_layer_cfg.draw("Projection Config File");
                    }

                    if (ImGui::Button("Add layer") && (selected_external_type == 2 || (projection_new_layer_file.isValid() &&
                                                                                       (selected_external_type == 0 ? 1 : projection_new_layer_cfg.isValid()))))
                    {
                        if (re_matchp(osm_url_regex, mapurl.c_str(), &osm_url_regex_len) || selected_external_type != 2)
                        {
                            urlgood = true;
                            auto genfun = [this](int)
                            {
                                is_opening_layer = true;
                                if (progress_pointer == nullptr)
                                    progress_pointer = new float;
                                general_progress = 0;
                                general_sum = 1;

                                std::shared_ptr<ExternalProjSource> new_layer_cfg = std::make_shared<ExternalProjSource>();
                                if (selected_external_type == 2)
                                    new_layer_cfg->name = "Tile Map";
                                else
                                    new_layer_cfg->name = projection_new_layer_name;

                                if (selected_external_type == 1)
                                    new_layer_cfg->path = projection_new_layer_cfg.getPath();
                                else if (selected_external_type == 2)
                                    new_layer_cfg->cfg = nlohmann::json::parse("{\"type\":\"mercator\"}");
                                else
                                    new_layer_cfg->cfg = nlohmann::json::parse("{\"type\":\"equirectangular\",\"tl_lon\":-180,\"tl_lat\":90,\"br_lon\":180,\"br_lat\":-90}");

                                int i = -1;
                                bool contains = false;
                                do
                                {
                                    contains = false;
                                    std::string curr_name = ((i + 1) == 0 ? new_layer_cfg->name : (new_layer_cfg->name + " #" + std::to_string(i + 1)));
                                    for (int i = 0; i < (int)projection_layers.size(); i++)
                                        if (projection_layers[i].name == curr_name)
                                            contains = true;
                                    i++;
                                } while (contains);
                                new_layer_cfg->name = (i == 0 ? new_layer_cfg->name : (new_layer_cfg->name + " #" + std::to_string(i)));

                                if (selected_external_type == 2)
                                {
                                    std::stringstream test(mapurl);
                                    std::string segment;
                                    std::vector<std::string> seglist;

                                    while (std::getline(test, segment, '/'))
                                    {
                                        seglist.push_back(segment);
                                    }
                                    logger->info("Generating tile map");
                                    tileMap tile_map(mapurl, satdump::user_path + "/osm_tiles/" + seglist[2] + "/");
                                    new_layer_cfg->img = tile_map.getMapImage({-85.06, -180}, {85.06, 180}, projection_osm_zoom, progress_pointer).to16bits();
                                }
                                else
                                    new_layer_cfg->img.load_img(projection_new_layer_file.getPath());

                                projections_external_sources.push_back(new_layer_cfg);

                                if (selected_external_type == 2)
                                    selected_external_type = 0;

                                progress_pointer = nullptr;
                                projections_should_refresh = true;
                                is_opening_layer = false;
                            };
                            ui_thread_pool.push(genfun);
                            ImGui::CloseCurrentPopup();
                        }
                        else
                        {
                            urlgood = false;
                        }
                    }
                    ImGui::SameLine();

                    if (ImGui::Button("Cancel"))
                        ImGui::CloseCurrentPopup();

                    ImGui::EndPopup();
                }
            }

            if (ImGui::BeginListBox("##projectionslistbox", ImVec2(ImGui::GetWindowWidth(), 300 * ui_scale)))
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
                    ImGui::Text("%s", label.c_str());
                    if (ImGui::IsItemHovered() && layer.type == 0)
                    {
                        ImGui::SetTooltip("%s", layer.viewer_prods->dataset_name.c_str());
                    }
                    ImGui::SameLine(ImGui::GetWindowWidth() - 70 * ui_scale);
                    ImGui::SetCursorPosY(ImGui::GetCursorPos().y - 2 * ui_scale);
                    ImGui::Checkbox(std::string("##enablelayer" + layer.name + std::to_string(i)).c_str(), &layer.enabled);
                    // if (layer.type == 1)
                    {
                        if (disable_buttons)
                            ImGui::BeginDisabled();
                        // Closing button
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(ImGui::GetCursorPos().y - 2 * ui_scale);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 0, 0, 255));
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                        if (ImGui::Button(std::string(u8"\uf00d##layerdelete" + layer.name + std::to_string(i)).c_str()))
                        {
                            if (layer.type == 1 || layer.type == 2)
                            {
                                for (int f = 0; f < (int)projections_external_sources.size(); f++)
                                    if (projections_external_sources[f]->name == projection_layers[i].name)
                                        projections_external_sources.erase(projections_external_sources.begin() + f);

                                projection_layers.erase(projection_layers.begin() + i);
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
                        if (projections_are_generating && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                            ImGui::SetTooltip("Wait for the processing to finish!");
                        ImGui::PopStyleColor();
                        ImGui::PopStyleColor();
                        if (disable_buttons)
                            ImGui::EndDisabled();
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

                        ImGui::Text("%s", label.c_str());
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
            if (!(disable_buttons || disable_add_layer))
                style::beginDisabled();

            ImGui::ProgressBar((general_progress + (progress_pointer == nullptr ? 0 : *(progress_pointer))) / general_sum);

            if (!(disable_buttons || disable_add_layer))
                style::endDisabled();

            if (disable_buttons || projection_layers.size() == 0)
                style::beginDisabled();
            if (ImGui::Button("Generate Projection##layers"))
            {
                ui_thread_pool.push([this](int)
                                    { 
                    logger->info("Update projection...");
                    generateProjectionImage();
                    logger->info("Done"); });
            }
            if (disable_buttons || projection_layers.size() == 0)
                style::endDisabled();
        }
        if (ImGui::CollapsingHeader("Overlay##viewerpojoverlay"))
        {
            projection_overlay_handler.drawUI();
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
        general_progress = 0;
        general_sum = 0;

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
        else if (projections_current_selected_proj == 4)
        {
            cfg["type"] = "azeq";
            cfg["lon"] = projections_azeq_lon;
            cfg["lat"] = projections_azeq_lat;
        }

        // Setup final image
        projected_image_result.init(projections_image_width, projections_image_height, 3);
        projected_image_result.init_font(resources::getResourcePath("fonts/font.ttf"));

        for (int i = projection_layers.size() - 1; i >= 0; i--)
        {
            ProjectionLayer &layer = projection_layers[i];
            if (!layer.enabled)
                continue;
            general_sum++;
        }
        general_sum += projection_overlay_handler.enabled();

        // Generate all layers
        std::vector<image::Image<uint16_t>> layers_images;

        for (int i = projection_layers.size() - 1; i >= 0; i--)
        {
            ProjectionLayer &layer = projection_layers[i];
            if (!layer.enabled)
                continue;
            if (progress_pointer == nullptr)
                progress_pointer = &layer.progress;

            if (layer.type == 0)
            {
                layer.viewer_prods->handler->updateProjection(projections_image_width, projections_image_height, cfg, &layer.progress);
                layers_images.push_back(layer.viewer_prods->handler->getProjection());
            }
            else
            {
                auto img = projectExternal(projections_image_width, projections_image_height, cfg, layer.external, &layer.progress);
                // img.to_rgb();
                layers_images.push_back(img);
            }
            general_progress++;
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

        // Free up memory
        layers_images.clear();

        // Setup projection to draw stuff on top
        auto proj_func = satdump::reprojection::setupProjectionFunction(projections_image_width, projections_image_height, cfg, {});

        // Draw map borders
        projection_overlay_handler.apply(projected_image_result, proj_func, &general_progress);

        // Update ImageView
        projection_image_widget.update(projected_image_result);
        general_sum = 1;
        general_progress = 1;
        progress_pointer = nullptr;
        projections_are_generating = false;
    }

    image::Image<uint16_t> ViewerApplication::projectExternal(int width, int height, nlohmann::json tcfg, std::shared_ptr<ExternalProjSource> ep, float *progress)
    {
        reprojection::ReprojectionOperation op;

        if (ep->path.size() > 0)
            op.source_prj_info = loadJsonFile(ep->path);
        else
            op.source_prj_info = ep->cfg;

        if (!op.source_prj_info.contains("type")) // Just in case...
            return image::Image<uint16_t>(width, height, 4);

        op.target_prj_info = tcfg;
        op.img = ep->img;
        op.output_width = width;
        op.output_height = height;
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
                if (projections_external_sources[i]->name == lay.name)
                    contains = true;

            if (!contains)
                projection_layers.insert(projection_layers.begin(), {projections_external_sources[i]->name, 1, nullptr, projections_external_sources[i]});
        }

        // Check for *new* products layers
        for (int i = 0; i < (int)products_and_handlers.size(); i++)
        {
            if (products_and_handlers[i]->handler->canBeProjected() && products_and_handlers[i]->handler->shouldProject())
            {
                std::string label = products_and_handlers[i]->products->instrument_name;

                bool contains = false;
                for (ProjectionLayer &lay : projection_layers)
                {
                    if (lay.type != 0)
                        continue;

                    if (label == lay.name && products_and_handlers[i].get() == lay.viewer_prods.get())
                        contains = true;
                }

                if (!contains)
                    projection_layers.insert(projection_layers.begin(), {label, 0, products_and_handlers[i], nullptr});
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
                if (products_and_handlers[i]->handler->canBeProjected() && products_and_handlers[i]->handler->shouldProject())
                    if (products_and_handlers[i]->products->instrument_name == lay.name && products_and_handlers[i].get() == lay.viewer_prods.get())
                        contains = true;

            if (!contains)
            {
                projection_layers.erase(projection_layers.begin() + i);
                goto recheck;
            }
        }
    }
}
