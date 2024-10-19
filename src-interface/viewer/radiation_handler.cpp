#include "radiation_handler.h"
#include "imgui/pfd/pfd_utils.h"
#include "resources.h"
#include "core/style.h"
#include "common/widgets/themed_widgets.h"
#include "main_ui.h"

#include "common/image/meta.h"

namespace satdump
{
    void RadiationViewerHandler::init()
    {
        products = (RadiationProducts *)ViewerHandler::products;

        for (int c = 0; c < (int)products->channel_counts.size(); c++)
            select_channel_image_str += products->get_channel_name(c) + '\0';

        update();
    }

    RadiationViewerHandler::~RadiationViewerHandler()
    {
        handler_thread_pool.stop();
        for (int i = 0; i < handler_thread_pool.size(); i++)
        {
            if (handler_thread_pool.get_thread(i).joinable())
                handler_thread_pool.get_thread(i).join();
        }
    }

    void RadiationViewerHandler::update()
    {
        if (selected_visualization_id == 0)
        {
            handler_thread_pool.clear_queue();
            handler_thread_pool.push([this](int)
                                     {   async_image_mutex.lock();
                                    is_updating = true;
                                    logger->info("Update map...");

                                    RadiationMapCfg cfg;
                                    cfg.channel = select_channel_image_id + 1;
                                    cfg.radius = 5;
                                    cfg.min = map_min;
                                    cfg.max = map_max;
                                    map_img = make_radiation_map(*products, cfg);
                                    image_view.update(map_img);

                                    logger->info("Done");
                                    is_updating = false;
                                    async_image_mutex.unlock(); });
        }
        else if (selected_visualization_id == 1)
        {
            graph_values.clear();
            graph_values.resize(products->channel_counts.size());
            for (int c = 0; c < (int)products->channel_counts.size(); c++)
            {
                for (int i = 0; i < (int)products->channel_counts[c].size(); i++)
                    graph_values[c].push_back(products->channel_counts[c][i]);
            }
        }
    }

    void RadiationViewerHandler::drawMenu()
    {
        if (ImGui::CollapsingHeader("View"))
        {
            if (ImGui::RadioButton(u8"\uf84c   Map", &selected_visualization_id, 0))
                update();
            if (ImGui::RadioButton(u8"\uf437   Graph", &selected_visualization_id, 1))
                update();

            if (selected_visualization_id == 0)
            {
                if (ImGui::Combo("###mapchannelcomboid", &select_channel_image_id, select_channel_image_str.c_str()))
                    update();
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);
                if (ImGui::DragInt("##Min", &map_min, 1.0f, 0, 255, "Min: %d", ImGuiSliderFlags_AlwaysClamp))
                    update();
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);
                if (ImGui::DragInt("##Max", &map_max, 1.0f, 0, 255, "Max: %d", ImGuiSliderFlags_AlwaysClamp))
                    update();
            }

            bool save_disabled = selected_visualization_id != 0 || is_updating;
            if (save_disabled)
                style::beginDisabled();

            if (ImGui::Button("Save"))
            {
                handler_thread_pool.push([this](int)
                                         {   async_image_mutex.lock();
                        is_updating = true;
                        logger->info("Saving Image...");
                        std::string default_path = config::main_cfg["satdump_directories"]["default_image_output_directory"]["value"].get<std::string>();
                        std::string saved_at = save_image_dialog(products->instrument_name + "_map", default_path, "Save Map", &map_img, &viewer_app->save_type); 

                        if (saved_at == "")
                            logger->info("Save cancelled");
                        else
                            logger->info("Saved current map at %s", saved_at.c_str());
                        is_updating = false;
                        async_image_mutex.unlock(); });
            }
            if (save_disabled)
                style::endDisabled();

            if (!canBeProjected())
                style::beginDisabled();
            if (ImGui::Button("Add to Projections"))
                addCurrentToProjections();
            ImGui::SameLine();
            proj_notif.draw();
            if (!canBeProjected())
                style::endDisabled();
        }
    }

    void RadiationViewerHandler::drawContents(ImVec2 win_size)
    {
        if (selected_visualization_id == 0)
        {
            image_view.draw(win_size);
        }
        else if (selected_visualization_id == 1)
        {
            ImGui::BeginChild("RadiationPlot");
            for (int i = 0; i < (int)products->channel_counts.size(); i++)
            {
                ImGui::BeginChild(("RadiationPlotChild##" + std::to_string(i)).c_str(), ImVec2(ImGui::GetWindowWidth(), 50 * ui_scale));
                widgets::ThemedPlotLines(style::theme.plot_bg.Value, products->get_channel_name(i).c_str(), graph_values[i].data(),
                                         graph_values[i].size(), 0, NULL, 0, 255, ImVec2(ImGui::GetWindowWidth() - 100 * ui_scale, 30 * ui_scale));
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::EndChild();
            }
            ImGui::EndChild();
        }
    }

    float RadiationViewerHandler::drawTreeMenu()
    {
        return 0;
    }

    bool RadiationViewerHandler::canBeProjected()
    {
        return selected_visualization_id == 0;
    }

    void RadiationViewerHandler::addCurrentToProjections()
    {
        if (canBeProjected())
        {
            try
            {
                RadiationMapCfg cfg;
                cfg.channel = select_channel_image_id + 1;
                cfg.radius = 5;
                cfg.min = map_min;
                cfg.max = map_max;
                auto map_img = make_radiation_map(*products, cfg, true);

                double tl_lon = -180;
                double tl_lat = 90;
                double br_lon = 180;
                double br_lat = -90;

                nlohmann::json proj_cfg;
                proj_cfg["type"] = "equirec";
                proj_cfg["offset_x"] = tl_lon;
                proj_cfg["offset_y"] = tl_lat;
                proj_cfg["scalar_x"] = (br_lon - tl_lon) / double(map_img.width());
                proj_cfg["scalar_y"] = (br_lat - tl_lat) / double(map_img.height());

                image::set_metadata_proj_cfg(map_img, proj_cfg);

                // Create projection title
                std::string timestring, object_name, instrument_name, composite_name;
                composite_name = "Channel " + std::to_string(select_channel_image_id);
                if (products->has_timestamps(select_channel_image_id))
                    timestring = "[" + timestamp_to_string(get_median(products->get_timestamps(select_channel_image_id))) + "] ";
                else
                    timestring = "";
                if (products->has_tle())
                    object_name = products->get_tle().name;
                else
                    object_name = "";
                if (timestring != "" || object_name != "")
                    object_name += "\n";
                if (instrument_cfg.contains("name"))
                    instrument_name = instrument_cfg["name"];
                else
                    instrument_name = products->instrument_name;

                viewer_app->projection_layers.push_front({timestring + object_name + instrument_name + " - " + composite_name, map_img});

                //                if (rotate_image)
                //                    viewer_app->projection_layers.front().img.mirror(true, true);
                proj_notif.set_message(style::theme.green, "Added!");
            }
            catch (std::exception &e)
            {
                logger->error("Could not project image! %s", e.what());
            }
        }
        else
        {
            logger->error("Current image can't be projected!");
        }
    }
}
