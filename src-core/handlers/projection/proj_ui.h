#pragma once

#include "imgui/imgui.h"
#include "nlohmann/json.hpp"
#include "projection/projection.h"

namespace satdump
{
    namespace proj
    {
        // TODOREWORK allow setting proj as well, and redo this better...
        class ProjectionConfigUI
        {
        public:
            int projections_image_width = 2048;
            int projections_image_height = 1024;

            int projections_mode_radio = 0;

            bool projection_auto_mode = false, projection_auto_scale_mode = false;
            double projection_autoscale_x = 0.016, projection_autoscale_y = 0.016;

            int projections_current_selected_proj = 0;
            /////////////
            float projections_equirectangular_tl_lon = -180;
            float projections_equirectangular_tl_lat = 90;
            float projections_equirectangular_br_lon = 180;
            float projections_equirectangular_br_lat = -90;
            /////////////
            float projections_utm_center_lon = 0;
            float projections_utm_offset_y = 0;
            float projections_utm_scale = 2400;
            int projections_utm_zone = 30;
            bool projections_utm_south = false;
            /////////////
            float projections_stereo_center_lon = 0;
            float projections_stereo_center_lat = 0;
            float projections_stereo_scale = 2400;
            /////////////
            float projections_tpers_lon = 0;
            float projections_tpers_lat = 0;
            float projections_tpers_alt = 30000000;
            float projections_tpers_ang = 0;
            float projections_tpers_azi = 0;
            float projections_tpers_scale = 8000;
            /////////////
            float projections_azeq_lon = 0;
            float projections_azeq_lat = 90;

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProjectionConfigUI, projections_image_width, projections_image_height, projections_mode_radio, projection_auto_mode, projection_auto_scale_mode,
                                           projection_autoscale_x, projection_autoscale_y, projections_current_selected_proj, projections_equirectangular_tl_lon, projections_equirectangular_tl_lat,
                                           projections_equirectangular_br_lon, projections_equirectangular_br_lat, projections_utm_center_lon, projections_utm_offset_y, projections_utm_scale,
                                           projections_utm_zone, projections_utm_south, projections_stereo_center_lon, projections_stereo_center_lat, projections_stereo_scale, projections_tpers_lon,
                                           projections_tpers_lat, projections_tpers_alt, projections_tpers_ang, projections_tpers_azi, projections_tpers_scale, projections_azeq_lon,
                                           projections_azeq_lat);

        public:
            nlohmann::json get_proj()
            {
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
                    cfg["scale"] = projections_utm_scale;
                    cfg["zone"] = projections_utm_zone;
                    cfg["south"] = projections_utm_south;
                    cfg["offset_y"] = projections_utm_offset_y;
                }
                else if (projections_current_selected_proj == 2)
                {
                    cfg["type"] = "stereo";
                    cfg["center_lon"] = projections_stereo_center_lon;
                    cfg["center_lat"] = projections_stereo_center_lat;
                    cfg["scale"] = projections_stereo_scale;
                    cfg["width"] = projections_image_width;
                    cfg["height"] = projections_image_height;
                }
                else if (projections_current_selected_proj == 3)
                {
                    cfg["type"] = "tpers";
                    cfg["center_lon"] = projections_tpers_lon;
                    cfg["center_lat"] = projections_tpers_lat;
                    cfg["altitude"] = projections_tpers_alt;
                    cfg["tilt"] = projections_tpers_ang;
                    cfg["azimuth"] = projections_tpers_azi;
                    cfg["scale"] = projections_tpers_scale;
                    cfg["width"] = projections_image_width;
                    cfg["height"] = projections_image_height;
                }
                /*else if (projections_current_selected_proj == 4)
                {
                    cfg["type"] = "azeq";
                    cfg["lon"] = projections_azeq_lon;
                    cfg["lat"] = projections_azeq_lat;
                }*/

                // Automatic projection settings!
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

                return cfg;
            }

            void drawUI()
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
                ImGui::Combo("##targetproj", &projections_current_selected_proj,
                             "Equirectangular\0"
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

                if (projections_current_selected_proj == 0 || projections_current_selected_proj == 2)
                {
                    ImGui::Checkbox("Auto Mode###pojautomode", &projection_auto_mode);
                    ImGui::Checkbox("Auto Scale Mode##projautoscalemode", &projection_auto_scale_mode);
                    if (projection_auto_scale_mode)
                    {
                        ImGui::InputDouble("Scale X (m/px)##projscalexauto", &projection_autoscale_x);
                        ImGui::InputDouble("Scale Y (m/px)##projscalexauto", &projection_autoscale_y);
                    }
                }
            }
        };
    } // namespace proj
} // namespace satdump