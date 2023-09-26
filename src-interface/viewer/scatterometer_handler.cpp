#include "scatterometer_handler.h"
#include "core/config.h"
#include "resources.h"
#include "common/projection/reprojector.h"
#include "core/style.h"
#include "common/map/map_drawer.h"
#include "imgui/pfd/pfd_utils.h"

#ifdef _MSC_VER
#include <direct.h>
#endif

namespace satdump
{
    void ScatterometerViewerHandler::init()
    {
        products = (ScatterometerProducts *)ViewerHandler::products;

        if (products->get_scatterometer_type() == products->SCAT_TYPE_ASCAT)
            current_scat_type = SCAT_ASCAT;

        if (current_scat_type == SCAT_ASCAT)
        {
            select_channel_image_str += std::string("Channel 1 (Back Right)") + '\0';
            select_channel_image_str += std::string("Channel 2 (Down Left)") + '\0';
            select_channel_image_str += std::string("Channel 3 (Back Left)") + '\0';
            select_channel_image_str += std::string("Channel 4 (Front Right)") + '\0';
            select_channel_image_str += std::string("Channel 5 (Back Left)") + '\0';
            select_channel_image_str += std::string("Channel 6 (Front Left)") + '\0';

            ascat_select_channel_image_str += std::string("Back") + '\0';
            ascat_select_channel_image_str += std::string("Down") + '\0';
            ascat_select_channel_image_str += std::string("Front") + '\0';
        }
        else
        {
            for (int c = 0; c < (int)products->get_channel_cnt(); c++)
                select_channel_image_str += "Channel " + std::to_string(c + 1) + '\0';
        }

        update();
    }

    void ScatterometerViewerHandler::update()
    {
        if (selected_visualization_id == 0)
        {
            GrayScaleScatCfg cfg;
            cfg.channel = select_channel_image_id;
            cfg.min = scat_grayscale_min;
            cfg.max = scat_grayscale_max;
            auto img = make_scatterometer_grayscale(*products, cfg);
            image_view.update(img);
        }
        else if (selected_visualization_id == 1)
        {
            GrayScaleScatCfg cfg;
            if (current_scat_type == SCAT_ASCAT)
                cfg.channel = ascat_select_channel_id;
            else
                cfg.channel = select_channel_image_id;
            cfg.min = scat_grayscale_min;
            cfg.max = scat_grayscale_max;
            current_image_proj.clear();
            current_img = make_scatterometer_grayscale_projs(*products, cfg, nullptr, &current_image_proj);

            if (map_overlay)
            {
                auto proj_func = satdump::reprojection::setupProjectionFunction(current_img.width(), current_img.height(), current_image_proj, {});
                logger->info("Drawing map overlay...");
                unsigned short color[3] = {(unsigned short)(color_borders.x * 65535.0f), (unsigned short)(color_borders.y * 65535.0f), (unsigned short)(color_borders.z * 65535.0f)};
                map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                               current_img,
                                               color,
                                               proj_func);
            }

            image_view.update(current_img);
        }
    }

    void ScatterometerViewerHandler::drawMenu()
    {
        if (ImGui::CollapsingHeader("Images"))
        {
            if (ImGui::RadioButton(u8"Raw Image", &selected_visualization_id, 0))
                update();
            if (ImGui::RadioButton(u8"Projected", &selected_visualization_id, 1))
                update();

            if (selected_visualization_id == 0 || selected_visualization_id == 1)
            {
                if (current_scat_type == SCAT_ASCAT && selected_visualization_id == 1)
                {
                    if (ImGui::Combo("###scatchannelcomboid", &ascat_select_channel_id, ascat_select_channel_image_str.c_str()))
                        update();
                }
                else
                {
                    if (ImGui::Combo("###scatchannelcomboid", &select_channel_image_id, select_channel_image_str.c_str()))
                        update();
                }

                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);
                if (ImGui::SliderInt("##MinScat", &scat_grayscale_min, 0, 1e7, "Min: %d", ImGuiSliderFlags_AlwaysClamp))
                    update();
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);
                if (ImGui::SliderInt("##MaxScat", &scat_grayscale_max, 0, 1e7, "Max: %d", ImGuiSliderFlags_AlwaysClamp))
                    update();
            }

            if (ImGui::Button("Save"))
            {
                std::string default_ext = satdump::config::main_cfg["satdump_general"]["image_format"]["value"].get<std::string>();
                std::string default_path = config::main_cfg["satdump_directories"]["default_image_output_directory"]["value"].get<std::string>();
#ifdef _MSC_VER
                if (default_path == ".")
                {
                    char* cwd;
                    cwd = _getcwd(NULL, 0);
                    if (cwd != 0)
                        default_path = cwd;
                }
                default_path += "\\";
#else
                default_path += "/";
#endif
                std::string ch_normal = std::to_string(select_channel_image_id);
                std::string ch_ascatp = std::to_string(ascat_select_channel_id);
                std::string default_name = default_path + products->instrument_name + "_" +
                    ((selected_visualization_id == 1 && current_scat_type == SCAT_ASCAT) ? ch_ascatp : ch_normal) + "." + default_ext;

#ifndef __ANDROID__
                auto result = pfd::save_file("Save Image", default_name, get_file_formats(default_ext));
                while (!result.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (result.result().size() > 0)
                {
                    std::string path = result.result();
                    logger->info("Saving current image at %s", path.c_str());
                    current_img.save_img(path);
                }
#else
                std::string path = "/storage/emulated/0/" + default_name;
                logger->info("Saving current image at %s", path.c_str());
                current_img.save_img("" + path);
#endif
            }
        }

        if (ImGui::CollapsingHeader("Map Overlay"))
        {
            if (selected_visualization_id != 1)
                style::beginDisabled();
            if (ImGui::Checkbox("Borders", &map_overlay))
                update();
            ImGui::SameLine();
            ImGui::ColorEdit3("##borders", (float *)&color_borders, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            if (selected_visualization_id != 1)
                style::endDisabled();
        }

        if (ImGui::CollapsingHeader("Projection"))
        {
            ImGui::BeginGroup();
            if (!canBeProjected())
                style::beginDisabled();
            ImGui::Checkbox("Project", &should_project);
            if (!canBeProjected())
                style::endDisabled();
            ImGui::EndGroup();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::BeginTooltip();
                if (selected_visualization_id != 1)
                    ImGui::TextColored(ImColor(255, 0, 0), "Select projection view first!");
                ImGui::EndTooltip();
            }
        }
    }

    void ScatterometerViewerHandler::drawContents(ImVec2 win_size)
    {
        if (selected_visualization_id == 0 || selected_visualization_id == 1)
        {
            image_view.draw(win_size);
        }
    }

    float ScatterometerViewerHandler::drawTreeMenu()
    {
        return 0;
    }

    bool ScatterometerViewerHandler::canBeProjected()
    {
        return products->has_proj_cfg() &&
               products->has_tle() &&
               products->has_proj_cfg() &&
               products->has_timestamps &&
               selected_visualization_id == 1;
    }

    bool ScatterometerViewerHandler::hasProjection()
    {
        return projection_ready && should_project;
    }

    bool ScatterometerViewerHandler::shouldProject()
    {
        return should_project;
    }

    void ScatterometerViewerHandler::updateProjection(int width, int height, nlohmann::json settings, float *progess)
    {
        if (canBeProjected())
        {
            reprojection::ReprojectionOperation op;
            op.source_prj_info = current_image_proj;
            op.target_prj_info = settings;
            op.img = current_img;
            op.output_width = width;
            op.output_height = height;
            // op.use_draw_algorithm = use_draw_proj_algo;
            // op.img_tle = products->get_tle();
            // op.img_tim = current_timestamps;
            reprojection::ProjectionResult res = reprojection::reproject(op, progess);
            projected_img = res.img;
            projection_ready = true;
        }
        else
        {
            logger->error("Current image can't be projected!");
        }
    }

    image::Image<uint16_t> &ScatterometerViewerHandler::getProjection()
    {
        return projected_img;
    }
}
