#include "viewer.h"
#include "imgui/imgui_stdlib.h"
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

#include "common/image/image_meta.h"
#include "common/projection/projs2/proj_json.h"
#include "common/widgets/spinner.h"

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
                                                                             "UTM (Mercator)\0"
                                                                             "Stereo\0"
                                                                             "Satellite (TPERS)\0"
                         //  "Azimuthal Equidistant\0"
            );

            if (projections_current_selected_proj == 0)
            {
                if (!projection_auto_mode)
                {
                    ImGui::Text("Top Left Coordinates :");
                    ImGui::InputFloat("Lat##tl", &projections_equirectangular_tl_lat);
                    ImGui::InputFloat("Lon##tl", &projections_equirectangular_tl_lon);
                    ImGui::Spacing();
                    ImGui::Text("Bottom Right Coordinates :");
                    ImGui::InputFloat("Lat##br", &projections_equirectangular_br_lat);
                    ImGui::InputFloat("Lon##br", &projections_equirectangular_br_lon);
                }
            }
            else if (projections_current_selected_proj == 1)
            {
                ImGui::InputInt("UTM Zone###projutmzone", &projections_utm_zone);
                if (projections_utm_zone > 60)
                    projections_utm_zone = 60;
                if (projections_utm_zone < 1)
                    projections_utm_zone = 1;
                ImGui::Checkbox("South###projutmsouth", &projections_utm_south);
                ImGui::InputFloat("Northing (m)##utm", &projections_utm_offset_y);
                ImGui::Spacing();
                ImGui::InputFloat("Scale (m/px)##utm", &projections_utm_scale);
            }
            else if (projections_current_selected_proj == 2)
            {
                ImGui::Text("Center Coordinates :");
                ImGui::InputFloat("Lat##stereo", &projections_stereo_center_lat);
                ImGui::InputFloat("Lon##stereo", &projections_stereo_center_lon);
                ImGui::Spacing();
                ImGui::InputFloat("Scale (m/px)##stereo", &projections_stereo_scale);
            }
            else if (projections_current_selected_proj == 3)
            {
                ImGui::Text("Center Coordinates :");
                ImGui::InputFloat("Lat##tpers", &projections_tpers_lat);
                ImGui::InputFloat("Lon##tpers", &projections_tpers_lon);
                ImGui::Spacing();
                ImGui::InputFloat("Altitude (m)##tpers", &projections_tpers_alt);
                ImGui::InputFloat("Angle##tpers", &projections_tpers_ang);
                ImGui::InputFloat("Azimuth##tpers", &projections_tpers_azi);
                ImGui::Spacing();
                ImGui::InputFloat("Scale##tpers", &projections_tpers_scale);
            }
            /*else if (projections_current_selected_proj == 4)
            {
                ImGui::Text("Center Coordinates :");
                ImGui::InputFloat("Lat##eqaz", &projections_azeq_lat);
                ImGui::InputFloat("Lon##eqaz", &projections_azeq_lon);
            }*/

            if (projections_current_selected_proj == 0)
            {
                ImGui::Checkbox("Auto Mode###pojautomode", &projection_auto_mode);
                ImGui::Checkbox("Auto Scale Mode##projautoscalemode", &projection_auto_scale_mode);
                if (projection_auto_scale_mode)
                {
                    ImGui::InputDouble("Scale X (m/px)##projscalexauto", &projection_autoscale_x);
                    ImGui::InputDouble("Scale Y (m/px)##projscalexauto", &projection_autoscale_y);
                }
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
            if (projections_are_generating)
            {
                ImGui::SameLine();
                widgets::Spinner("###spinner1", 8 * ui_scale, 3 * ui_scale, ImGui::GetColorU32(ImGuiCol_Text));
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

            ImGui::SameLine(ImGui::GetWindowWidth() - (projections_loading_new_layer ? 105 : 95) * ui_scale);
            if (disable_add_layer)
                style::beginDisabled();

            if (ImGui::Button("Add Layer##button"))
                ImGui::OpenPopup("Add Layer##popup", ImGuiPopupFlags_None);

            if (projections_loading_new_layer)
            {
                ImGui::SameLine();
                widgets::Spinner("###spinner1", 8 * ui_scale, 3 * ui_scale, ImGui::GetColorU32(ImGuiCol_Text));
            }

            if (disable_add_layer)
                style::endDisabled();

            {
                if (ImGui::BeginPopupModal("Add Layer##popup", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    ImGui::RadioButton("Equirectangular", &selected_external_type, 0);
                    style::beginDisabled();
                    ImGui::RadioButton("Tile Map (OSM)", &selected_external_type, 1);
                    style::endDisabled();
                    ImGui::RadioButton("GeoTIFF", &selected_external_type, 2);
                    ImGui::RadioButton("Other", &selected_external_type, 3);

                    if (selected_external_type == 0 ||
                        selected_external_type == 2 ||
                        selected_external_type == 3)
                    {
                        ImGui::InputText("Name", &projection_new_layer_name);
                        projection_new_layer_file.draw("Input Image");
                        if (selected_external_type == 3)
                            projection_new_layer_cfg.draw("Projection Config File");
                        ImGui::Checkbox("Normalize###normalizeinput", &projection_normalize_image);
                    }

                    if (ImGui::Button("Add layer"))
                    {
                        auto func = [this](int)
                        {
                            if ((selected_external_type == 0 || selected_external_type == 3) && projection_new_layer_file.isValid())
                            {
                                image::Image<uint16_t> equi_file;
                                equi_file.load_img(projection_new_layer_file.getPath());
                                if (equi_file.size() > 0)
                                {
                                    double tl_lon = -180;
                                    double tl_lat = 90;
                                    double br_lon = 180;
                                    double br_lat = -90;
                                    nlohmann::json proj_cfg;
                                    if (selected_external_type == 0)
                                    {
                                        proj_cfg["type"] = "equirec";
                                        proj_cfg["offset_x"] = tl_lon;
                                        proj_cfg["offset_y"] = tl_lat;
                                        proj_cfg["scalar_x"] = (br_lon - tl_lon) / double(equi_file.width());
                                        proj_cfg["scalar_y"] = (br_lat - tl_lat) / double(equi_file.height());
                                    }
                                    else if (selected_external_type == 3)
                                    {
                                        proj_cfg = loadJsonFile(projection_new_layer_cfg.getPath());
                                    }
                                    if (projection_normalize_image)
                                        equi_file.normalize();
                                    image::set_metadata_proj_cfg(equi_file, proj_cfg);
                                    projection_layers.push_back({projection_new_layer_name, equi_file});
                                }
                                else
                                    logger->error("Could not load image file!");
                            }
                            else if (selected_external_type == 2 && projection_new_layer_file.isValid())
                            {
                                image::Image<uint16_t> geotiff_file;
                                geotiff_file.load_tiff(projection_new_layer_file.getPath());
                                if (geotiff_file.size() > 0 && image::has_metadata_proj_cfg(geotiff_file))
                                {
                                    if (projection_normalize_image)
                                        geotiff_file.normalize();
                                    projection_layers.push_back({projection_new_layer_name, geotiff_file});
                                }
                                else
                                    logger->error("Could not load GeoTIFF. This may not be a TIFF file, or the projection settings are unsupported? If you think they should be supported, open an issue on GitHub.");
                            }

                            projections_loading_new_layer = false;
                        };

                        ui_thread_pool.push(func);
                        projections_loading_new_layer = true;

                        ImGui::CloseCurrentPopup();
                    }

#if 0
                    if (selected_external_type == 2)
                    {
                        ImGui::SliderInt("Zoom##osmsliderzoom", &projection_osm_zoom, 0, 6);
                        if (!urlgood)
                            ImGui::PushStyleColor(ImGuiCol_Text, style::theme.red.Value);

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
#endif
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
                    //                    if (layer.type == 0)
                    //                    {
                    //                        label = layer.viewer_prods->products->instrument_name;
                    //                        if (layer.viewer_prods->handler->instrument_cfg.contains("name"))
                    //                            label = layer.viewer_prods->handler->instrument_cfg["name"].get<std::string>();
                    //                    }
                    //                    else
                    label = layer.name;
                    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + 4 * ui_scale));
                    ImGui::Text("%s", label.c_str());
                    //                    if (ImGui::IsItemHovered() && layer.type == 0)
                    //                    {
                    //                        ImGui::SetTooltip("%s", layer.viewer_prods->dataset_name.c_str());
                    //                    }
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
                        ImGui::PushStyleColor(ImGuiCol_Text, style::theme.red.Value);
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
                        if (ImGui::Button(std::string(u8"\uf00d##layerdelete" + layer.name + std::to_string(i)).c_str()))
                        {
                            projection_layers.erase(projection_layers.begin() + i);
                            ImGui::PopStyleVar();
                            ImGui::PopStyleColor(2);
                            ImGui::EndGroup();
                            ImGui::PopID();
                            break;
                        }
                        if (projections_are_generating && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                            ImGui::SetTooltip("Wait for the processing to finish!");
                        ImGui::PopStyleVar();
                        ImGui::PopStyleColor(2);
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
        }
        if (ImGui::CollapsingHeader("Overlay##viewerpojoverlay"))
        {
            projection_overlay_handler.drawUI();
        }
    }

    void ViewerApplication::generateProjectionImage()
    {
        projections_are_generating = true;
        general_progress = 0;
        general_sum = 0;

        nlohmann::json cfg;

        if (projections_current_selected_proj == 0)
        {
            cfg["type"] = "equirec";
            cfg["offset_x"] = projections_equirectangular_tl_lon;
            cfg["offset_y"] = projections_equirectangular_tl_lat;
            cfg["scalar_x"] = (projections_equirectangular_br_lon - projections_equirectangular_tl_lon) / double(projections_image_width);
            cfg["scalar_y"] = (projections_equirectangular_br_lat - projections_equirectangular_tl_lat) / double(projections_image_height);
        }
        else if (projections_current_selected_proj == 1)
        {
            cfg["type"] = "utm";
            cfg["scalar_x"] = projections_utm_scale;
            cfg["scalar_y"] = -projections_utm_scale;
            cfg["offset_x"] = -projections_image_width * 0.5 * projections_utm_scale;
            cfg["offset_y"] = projections_image_height * 0.5 * projections_utm_scale + projections_utm_offset_y;
            cfg["zone"] = projections_utm_zone;
            cfg["south"] = projections_utm_south;
        }
        else if (projections_current_selected_proj == 2)
        {
            cfg["type"] = "stereo";
            cfg["lon0"] = projections_stereo_center_lon;
            cfg["lat0"] = projections_stereo_center_lat;
            cfg["scalar_x"] = projections_stereo_scale;
            cfg["scalar_y"] = -projections_stereo_scale;
            cfg["offset_x"] = -projections_image_width * 0.5 * projections_stereo_scale;
            cfg["offset_y"] = projections_image_height * 0.5 * projections_stereo_scale;
        }
        else if (projections_current_selected_proj == 3)
        {
            cfg["type"] = "tpers";
            cfg["lon0"] = projections_tpers_lon;
            cfg["lat0"] = projections_tpers_lat;
            cfg["altitude"] = projections_tpers_alt;
            cfg["tilt"] = projections_tpers_ang;
            cfg["azimuth"] = projections_tpers_azi;
            cfg["scalar_x"] = projections_tpers_scale;
            cfg["scalar_y"] = -projections_tpers_scale;
            cfg["offset_x"] = -projections_image_width * 0.5 * projections_tpers_scale;
            cfg["offset_y"] = projections_image_height * 0.5 * projections_tpers_scale;
        }
        /*else if (projections_current_selected_proj == 4)
        {
            cfg["type"] = "azeq";
            cfg["lon"] = projections_azeq_lon;
            cfg["lat"] = projections_azeq_lat;
        }*/

        // Automatic projection settings!
        if (projection_auto_mode && projections_current_selected_proj == 0)
        {
            reprojection::ProjBounds bounds;
            bounds.min_lon = 180;
            bounds.max_lon = -180;
            bounds.min_lat = 90;
            bounds.max_lat = -90;
            for (auto &layer : projection_layers)
            {
                if (!layer.enabled)
                    continue;
                auto boundshere = reprojection::determineProjectionBounds(layer.img);
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
                cfg["scale_x"] = projection_autoscale_x;
                cfg["scale_y"] = projection_autoscale_y;
            }
            else
            {
                cfg["width"] = projections_image_width;
                cfg["height"] = projections_image_height;
            }

            reprojection::tryAutoTuneProjection(bounds, cfg);
            if (cfg.contains("width"))
                projections_image_width = cfg["width"];
            if (cfg.contains("height"))
                projections_image_height = cfg["height"];

            logger->debug("\n%s", cfg.dump(4).c_str());
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

            int width = projections_image_width;
            int height = projections_image_height;

            reprojection::ReprojectionOperation op;

            if (!image::has_metadata_proj_cfg(layer.img)) // Just in case...
                continue;
            if (!image::get_metadata_proj_cfg(layer.img).contains("type")) // Just in case...
                continue;

            op.target_prj_info = cfg;
            op.img = layer.img;
            op.output_width = width;
            op.output_height = height;

            op.use_old_algorithm = layer.old_algo;

            image::Image<uint16_t> res = reprojection::reproject(op, progress_pointer);
            layers_images.push_back(res);

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

        logger->info("Applying overlays...");

        // Setup projection to draw stuff on top
        auto proj_func = satdump::reprojection::setupProjectionFunction(projections_image_width, projections_image_height, cfg, {});

        // Draw map borders
        projection_overlay_handler.clear_cache();
        projection_overlay_handler.apply(projected_image_result, proj_func, &general_progress);

        // Update ImageView
        projection_image_widget.update(projected_image_result);
        general_sum = 1;
        general_progress = 1;
        progress_pointer = nullptr;
        projections_are_generating = false;
    }
}
