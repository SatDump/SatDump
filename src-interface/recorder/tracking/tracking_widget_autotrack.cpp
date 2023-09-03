#include "tracking_widget.h"
#include "common/utils.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/tracking/tle.h"
#include "core/module.h"
#include "core/style.h"
#include "main_ui.h"

namespace satdump
{
    void TrackingWidget::processAutotrack()
    {
        if (autotrack_engaged)
        {
            upcoming_satellite_passes_mtx.lock();

            if (upcoming_satellite_passes_sel.size() > 0)
            {
                if (!autotrack_pass_has_started && getTime() > upcoming_satellite_passes_sel[0].aos_time)
                {
                    logger->critical("AOS!!!!!!!!!!!!!! %d", upcoming_satellite_passes_sel[0].norad);
                    tracking::TrackedObject obj;
                    for (auto &v : enabled_satellites)
                        if (v.norad == upcoming_satellite_passes_sel[0].norad)
                            obj = v;
                    aos_callback(upcoming_satellite_passes_sel[0], obj);
                    autotrack_pass_has_started = true;
                }
            }

            if (getTime() > upcoming_satellite_passes_sel[0].los_time && upcoming_satellite_passes_sel.size() > 0)
            {
                if (autotrack_pass_has_started)
                {
                    logger->critical("LOS!!!!!!!!!!!!!! %d", upcoming_satellite_passes_sel[0].norad);
                    tracking::TrackedObject obj;
                    for (auto &v : enabled_satellites)
                        if (v.norad == upcoming_satellite_passes_sel[0].norad)
                            obj = v;
                    los_callback(upcoming_satellite_passes_sel[0], obj);
                }
                autotrack_pass_has_started = false;
                updateAutotrackPasses(); // upcoming_satellite_passes_sel.erase(upcoming_satellite_passes_sel.begin());
                for (int i = 0; i < (int)general_tle_registry.size(); i++)
                    if (general_tle_registry[i].norad == upcoming_satellite_passes_sel[0].norad)
                        current_satellite = i;
                backend_needs_update = true;
            }

            upcoming_satellite_passes_mtx.unlock();
        }
    }

    void TrackingWidget::updateAutotrackPasses()
    {
        upcoming_satellite_passes_all.clear();

        double initial_time = getTime();

        for (tracking::TrackedObject &obj : enabled_satellites)
        {
            auto tle = general_tle_registry.get_from_norad(obj.norad).value();
            predict_orbital_elements_t *satellite_object_ = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());

            double current_time = initial_time;

            while (current_time < initial_time + 12 * 3600)
            {
                predict_observation next_aos, next_los;
                next_aos = next_los = predict_next_los(observer_station, satellite_object_, predict_to_julian_double(current_time));

                // Calculate the AOS before that LOS
                double next_aos_time_, next_los_time_;
                next_aos_time_ = next_los_time_ = predict_from_julian(next_los.time);
                do
                {
                    next_aos = predict_next_aos(observer_station, satellite_object_, predict_to_julian_double(next_aos_time_));
                    next_aos_time_ -= 10;
                } while (predict_from_julian(next_aos.time) >= next_los_time_);

                next_los_time_ = predict_from_julian(next_los.time);
                next_aos_time_ = predict_from_julian(next_aos.time);

                float max_el = 0;
                predict_position satellite_orbit2;
                predict_observation observation_pos2;
                double time_step = abs(next_los_time_ - next_aos_time_) / 50.0;
                for (double ctime = next_aos_time_; ctime <= next_los_time_; ctime += time_step)
                {
                    predict_orbit(satellite_object_, &satellite_orbit2, predict_to_julian_double(ctime));
                    predict_observe_orbit(observer_station, &satellite_orbit2, &observation_pos2);
                    if (observation_pos2.elevation * RAD_TO_DEG > max_el)
                        max_el = observation_pos2.elevation * RAD_TO_DEG;
                }

                if (max_el >= autotrack_min_elevation)
                    upcoming_satellite_passes_all.push_back({obj.norad,
                                                             next_aos_time_,
                                                             next_los_time_,
                                                             max_el});
                // logger->info("Pass of %s at AOS %s LOS %s elevation %.2f", tle.name.c_str(),
                //              timestamp_to_string(next_aos_time_).c_str(),
                //              timestamp_to_string(next_los_time_).c_str(),
                //              max_el);

                current_time = next_los_time_ + 1;
            }

            predict_destroy_orbital_elements(satellite_object_);
        }

        std::sort(upcoming_satellite_passes_all.begin(), upcoming_satellite_passes_all.end(),
                  [](tracking::SatellitePass &el1,
                     tracking::SatellitePass &el2)
                  {
                      return el1.aos_time < el2.aos_time;
                  });

        upcoming_satellite_passes_sel.clear();

        for (int i = 0; i < (int)upcoming_satellite_passes_all.size() - 1; i++)
        {
            auto &pass1 = upcoming_satellite_passes_all[i];
            auto &pass2 = upcoming_satellite_passes_all[i + 1];

            if (pass1.los_time > pass2.aos_time)
            {
                // logger->critical("Overlap : ");
                // logPass(pass1);
                // logPass(pass2);

                if (pass1.max_elevation > pass2.max_elevation)
                    upcoming_satellite_passes_sel.push_back(pass1);
                else
                    upcoming_satellite_passes_sel.push_back(pass2);
            }
            else
            {
                upcoming_satellite_passes_sel.push_back(pass1);
            }
        }

        // for (auto ppp : upcoming_satellite_passes_sel)
        // logger->debug("Pass of %s at AOS %s LOS %s elevation %.2f",
        //               general_tle_registry.get_from_norad(ppp.norad).value().name.c_str(),
        //               timestamp_to_string(ppp.aos_time).c_str(),
        //               timestamp_to_string(ppp.los_time).c_str(),
        //               ppp.max_elevation);
    }

    void TrackingWidget::renderAutotrackConfig()
    {
        upcoming_satellite_passes_mtx.lock();

        if (autotrack_engaged)
            style::beginDisabled();

        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 220 * ui_scale);
        ImGui::BeginGroup();
        ImGui::SetNextItemWidth(200 * ui_scale);
        if (ImGui::BeginListBox("##trackingavailablesatsbox"))
        {
            for (int i = 0; i < (int)satoptions.size(); i++)
                if (std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [i](tracking::TrackedObject &c)
                                 { return c.norad == general_tle_registry[i].norad; }) == enabled_satellites.end())
                    if (ImGui::Selectable(satoptions[i].c_str(), i == tracking_sats_menu_selected_1))
                        tracking_sats_menu_selected_1 = i;
            ImGui::EndListBox();
        }
        ImGui::SameLine();
        int curpos = ImGui::GetCursorPosY();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 15 * ui_scale);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50 * ui_scale);
        ImGui::BeginGroup();
        if (ImGui::Button(">>>"))
        {
            auto it = std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [this](tracking::TrackedObject &t)
                                   { return t.norad == general_tle_registry[tracking_sats_menu_selected_1].norad; });
            if (it == enabled_satellites.end())
                enabled_satellites.push_back({general_tle_registry[tracking_sats_menu_selected_1].norad});
        }
        if (ImGui::Button("<<<"))
        {
            auto it = std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [this](tracking::TrackedObject &t)
                                   { return t.norad == general_tle_registry[tracking_sats_menu_selected_2].norad; });
            if (it != enabled_satellites.end())
                enabled_satellites.erase(it);
        }
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::SetCursorPosY(curpos);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 + 20 * ui_scale);
        ImGui::SetNextItemWidth(200 * ui_scale);
        if (ImGui::BeginListBox("##trackingselectedsatsbox"))
        {
            for (int i = 0; i < (int)satoptions.size(); i++)
                if (std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [i](tracking::TrackedObject &c)
                                 { return c.norad == general_tle_registry[i].norad; }) != enabled_satellites.end())
                    if (ImGui::Selectable(satoptions[i].c_str(), i == tracking_sats_menu_selected_2))
                        tracking_sats_menu_selected_2 = i;
            ImGui::EndListBox();
        }
        ImGui::EndGroup();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 45 * ui_scale);
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::SetNextItemWidth(150 * ui_scale);
        ImGui::InputFloat("Minimum Elevation", &autotrack_min_elevation);
        if (ImGui::Button("Update Passes"))
        {
            updateAutotrackPasses();
        }
        if (autotrack_engaged)
            style::endDisabled();
        ImGui::SameLine();
        if (ImGui::Checkbox("Engage Autotrack", &autotrack_engaged))
        {
            updateAutotrackPasses();

            if (upcoming_satellite_passes_sel.size() > 0)
            {
                for (int i = 0; i < (int)general_tle_registry.size(); i++)
                    if (general_tle_registry[i].norad == upcoming_satellite_passes_sel[0].norad)
                        current_satellite = i;
                horizons_mode = false;
                backend_needs_update = true;
                autotrack_pass_has_started = false;
            }
            else
            {
                autotrack_engaged = false;
            }

            saveConfig();
        }

#if 1
        {
            ImGui::Spacing();
            int d_pplot_height = (enabled_satellites.size() * 20) * ui_scale;
            int d_pplot_size = ImGui::GetWindowContentRegionWidth();
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                     ImVec2(ImGui::GetCursorScreenPos().x + d_pplot_size, ImGui::GetCursorScreenPos().y + d_pplot_height + 20 * ui_scale),
                                     light_theme ? ImColor(255, 255, 255, 255) : ImColor::HSV(0, 0, 0));

            double current_time = getTime();
            time_t tttime = current_time;
            std::tm *timeReadable = gmtime(&tttime);
            int curr_hour = timeReadable->tm_hour;
            int offset = d_pplot_size / 12 * (timeReadable->tm_min/60.0);
            ImGui::Dummy(ImVec2(0, 0));
            for (int i = (timeReadable->tm_min < 30 ? 1 : 0); i < (timeReadable->tm_min < 30 ? 12 : 13); i++)
            {
                ImGui::SameLine();
                ImGui::SetCursorPosX(i * d_pplot_size / 12 - offset);
                ImGui::Text("%s%s%s", (curr_hour + i)%24 < 10 ? "0" : "", std::to_string((curr_hour + i) % 24).c_str(), ":00");
            }
            float sat_blk_height = ((float)d_pplot_height / (float)enabled_satellites.size());
            for (int i = 0; i < (int)enabled_satellites.size(); i++)
            {
                auto &norad = enabled_satellites[i].norad;
                int thsat_ys = sat_blk_height * i;
                int thsat_ye = sat_blk_height * i + sat_blk_height;

                bool first_pass = true;

                for (auto &cpass : upcoming_satellite_passes_all)
                {
                    if (cpass.norad == norad)
                    {
                        double cpass_xs = ((cpass.aos_time - current_time) / (12.0 * 3600.0)) * d_pplot_size;
                        double cpass_xe = ((cpass.los_time - current_time) / (12.0 * 3600.0)) * d_pplot_size;

                        std::string name = general_tle_registry.get_from_norad(norad)->name;

                        if (cpass_xs < 0)
                            cpass_xs = 0;
                        if (cpass_xe > d_pplot_size)
                            cpass_xe = d_pplot_size;
                        if (cpass_xs > d_pplot_size)
                            cpass_xs = d_pplot_size;

                        auto color = ImColor::HSV(fmod(norad, 10) / 10.0, 1, 1);
                        draw_list->AddRectFilled(ImVec2(ImGui::GetCursorScreenPos().x + cpass_xs, ImGui::GetCursorScreenPos().y + thsat_ys + 1),
                                                 ImVec2(ImGui::GetCursorScreenPos().x + cpass_xe, ImGui::GetCursorScreenPos().y + thsat_ye),
                                                 color, 3);

                        if (first_pass)
                        {
                            draw_list->AddText(ImVec2(ImGui::GetCursorScreenPos().x + cpass_xe + 5 * ui_scale,
                                                      ImGui::GetCursorScreenPos().y + thsat_ys + (sat_blk_height / 2) - 8 * ui_scale),
                                               color, name.c_str());
                            first_pass = false;
                        }

                        if (ImGui::IsMouseHoveringRect(ImVec2(ImGui::GetCursorScreenPos().x + cpass_xs, ImGui::GetCursorScreenPos().y + thsat_ys),
                                                       ImVec2(ImGui::GetCursorScreenPos().x + cpass_xe, ImGui::GetCursorScreenPos().y + thsat_ye)))
                            ImGui::SetTooltip("%s\nAOS : %s\nLOS : %s\nEl : %.2f",
                                              name.c_str(),
                                              timestamp_to_string(cpass.aos_time).c_str(),
                                              timestamp_to_string(cpass.los_time).c_str(),
                                              cpass.max_elevation);
                    }
                }

                draw_list->AddLine(ImVec2(ImGui::GetCursorScreenPos().x + 0, ImGui::GetCursorScreenPos().y + thsat_ys),
                                   ImVec2(ImGui::GetCursorScreenPos().x + d_pplot_size, ImGui::GetCursorScreenPos().y + thsat_ys),
                                   ImColor(100, 100, 100, 255));
                draw_list->AddLine(ImVec2(ImGui::GetCursorScreenPos().x + 0, ImGui::GetCursorScreenPos().y + thsat_ye),
                                   ImVec2(ImGui::GetCursorScreenPos().x + d_pplot_size, ImGui::GetCursorScreenPos().y + thsat_ye),
                                   ImColor(100, 100, 100, 255));

                if (autotrack_engaged)
                {
                    for (auto &cpass : upcoming_satellite_passes_sel)
                    {
                        if (cpass.norad == norad)
                        {
                            double cpass_xs = ((cpass.aos_time - current_time) / (12.0 * 3600.0)) * d_pplot_size;
                            double cpass_xe = ((cpass.los_time - current_time) / (12.0 * 3600.0)) * d_pplot_size;

                            if (cpass_xs < 0)
                                cpass_xs = 0;
                            if (cpass_xe > d_pplot_size)
                                cpass_xe = d_pplot_size;
                            if (cpass_xs > d_pplot_size)
                                cpass_xs = d_pplot_size;

                            auto color = ImColor(255, 255, 255, 255);
                            draw_list->AddRect(ImVec2(ImGui::GetCursorScreenPos().x + cpass_xs, ImGui::GetCursorScreenPos().y + thsat_ys),
                                               ImVec2(ImGui::GetCursorScreenPos().x + cpass_xe, ImGui::GetCursorScreenPos().y + thsat_ye),
                                               color, 3);
                        }
                    }
                }
            }

            ImGui::Dummy(ImVec2(d_pplot_size + 3 * ui_scale, d_pplot_height + 3 * ui_scale));
        }
#endif
        if (autotrack_engaged)
            style::beginDisabled();

        if (ImGui::BeginTable("##trackingwidgettrackeobjectsconfig", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
        {
            for (auto &cpass : enabled_satellites)
            {
                auto color = ImColor::HSV(fmod(cpass.norad, 10) / 10.0, 1, 1);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::SetNextItemWidth(100*ui_scale);
                ImGui::TextColored(color, "%s", general_tle_registry.get_from_norad(cpass.norad)->name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(120*ui_scale);
                ImGui::InputDouble(((std::string) "Frequency##objcfgfreq1" + std::to_string(cpass.norad)).c_str(), &cpass.frequency, 0, 0, "%f MHz");
                ImGui::TableSetColumnIndex(2);
                ImGui::SetNextItemWidth(100*ui_scale);
                ImGui::Checkbox(((std::string) "Record##objcfgfreq2" + std::to_string(cpass.norad)).c_str(), &cpass.record);
                // ImGui::TableSetColumnIndex(3);
                ImGui::Checkbox(((std::string) "Live##objcfgfreq3" + std::to_string(cpass.norad)).c_str(), &cpass.live);
                ImGui::TableSetColumnIndex(3);
                ImGui::SetNextItemWidth(300*ui_scale);
                ImGui::PushID(cpass.norad);
                if (ImGui::BeginCombo("##pipelinesel", cpass.pipeline_selector->get_name(cpass.pipeline_selector->pipeline_id).c_str(), ImGuiComboFlags_HeightLarge))
                {
                    cpass.pipeline_selector->renderSelectionBox(300 * ui_scale);
                    ImGui::EndCombo();
                }
                if (ImGui::BeginCombo("##params", "Configure..."))
                {
                    cpass.pipeline_selector->renderParamTable();
                    ImGui::EndCombo();
                }
                ImGui::PopID();
                // ImGui::InputText(((std::string) "Pipeline##objcfgfreq4" + std::to_string(cpass.norad)).c_str(), &cpass.pipeline_name);
            }
            ImGui::EndTable();
        }

        if (autotrack_engaged)
            style::endDisabled();
#if 0
        if (ImGui::BeginTable("##trackingwidgettable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            for (auto &cpass : upcoming_satellite_passes_all)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", general_tle_registry.get_from_norad(cpass.norad)->name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f", cpass.max_elevation);
            }

            ImGui::EndTable();
        }
#endif

        upcoming_satellite_passes_mtx.unlock();
    }
}
