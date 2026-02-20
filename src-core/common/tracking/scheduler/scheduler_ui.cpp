#include "init.h"
#define NOMINMAX
#include <cmath>

#include "common/dsp_source_sink/format_notated.h"
#include "common/widgets/frequency_input.h"
#include "core/resources.h"
#include "core/style.h"
#include "image/text.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"
#include "scheduler.h"
#include "utils/color.h"
#include "utils/format.h"
#include "utils/string.h"
#include "utils/time.h"

namespace satdump
{
    void AutoTrackScheduler::renderAutotrackConfig(double curr_time)
    {
        if (!has_tle)
            return;

        // Thread Safety
        upcoming_satellite_passes_mtx.lock();
        auto tle_registry = db_tle->all;
        int num_objects = std::min(tle_registry.size(), satoptions.size()); // These can temporarily get out of sync on update

        if (autotrack_engaged)
            style::beginDisabled();

        ImGuiStyle &imgui_style = ImGui::GetStyle();
        float curpos = ImGui::GetCursorPosY();
        ImGui::SetNextItemWidth(200 * ui_scale);
        ImGui::InputTextWithHint("##trackingavailablesatssearch", u8"\uf422   Search All Satellites", &availablesatssearch);
        ImGui::BeginGroup();
        ImGui::SetNextItemWidth(200 * ui_scale);
        if (ImGui::BeginListBox("##trackingavailablesatsbox"))
        {
            for (int i = 0; i < num_objects; i++)
                if (std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [i, &tle_registry](TrackedObject &c) { return c.norad == (tle_registry)[i].norad; }) == enabled_satellites.end())
                    if (availablesatssearch.size() == 0 || satdump::isStringPresent(satoptions[i], availablesatssearch))
                    {
                        if (ImGui::Selectable(satoptions[i].c_str(), i == tracking_sats_menu_selected_1))
                            tracking_sats_menu_selected_1 = i;

                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                        {
                            auto it = std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [i, &tle_registry](TrackedObject &t) { return t.norad == (tle_registry)[i].norad; });
                            if (it == enabled_satellites.end())
                                enabled_satellites.push_back({(tle_registry)[i].norad});
                        }
                    }
            ImGui::EndListBox();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50 * ui_scale);
        ImGui::BeginGroup();
        if (ImGui::Button(">>>"))
        {
            auto it =
                std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [this, &tle_registry](TrackedObject &t) { return t.norad == (tle_registry)[tracking_sats_menu_selected_1].norad; });
            if (it == enabled_satellites.end())
                enabled_satellites.push_back({(tle_registry)[tracking_sats_menu_selected_1].norad});
        }
        if (ImGui::Button("<<<"))
        {
            auto it =
                std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [this, &tle_registry](TrackedObject &t) { return t.norad == (tle_registry)[tracking_sats_menu_selected_2].norad; });
            if (it != enabled_satellites.end())
                enabled_satellites.erase(it);
        }

        ImGui::EndGroup();
        float selected_offset = 200 * ui_scale + ImGui::GetItemRectSize().x + imgui_style.ItemSpacing.x * 2;
        ImGui::SetCursorPos({selected_offset, curpos});
        ImGui::SetNextItemWidth(200 * ui_scale);
        ImGui::InputTextWithHint("##trackingselectedsatssearch", u8"\uf422   Search Selected", &selectedsatssearch);
        ImGui::SetCursorPosX(selected_offset);
        ImGui::SetNextItemWidth(200 * ui_scale);
        if (ImGui::BeginListBox("##trackingselectedsatsbox"))
        {
            for (int i = 0; i < num_objects; i++)
                if (std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [i, &tle_registry](TrackedObject &c) { return c.norad == (tle_registry)[i].norad; }) != enabled_satellites.end())
                    if (selectedsatssearch.size() == 0 || satdump::isStringPresent(satoptions[i], selectedsatssearch))
                    {
                        if (ImGui::Selectable(satoptions[i].c_str(), i == tracking_sats_menu_selected_2))
                            tracking_sats_menu_selected_2 = i;

                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                        {
                            auto it = std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [i, &tle_registry](TrackedObject &t) { return t.norad == (tle_registry)[i].norad; });
                            if (it != enabled_satellites.end())
                                enabled_satellites.erase(it);
                        }
                    }

            ImGui::EndListBox();
        }
        ImGui::EndGroup();

        float selection_box_bottom = ImGui::GetCursorPosY();
        float elev_width = selected_offset + 200 * ui_scale + imgui_style.ItemSpacing.x;
        ImGui::SetCursorPos({elev_width, curpos});
        ImGui::BeginGroup();
        ImGui::SeparatorText("Scheduler Options");
        ImGui::Checkbox("Multi Mode", &autotrack_cfg.multi_mode);
        ImGui::SameLine();
        elev_width = ImGui::GetCursorPosX() - elev_width;
        ImGui::Checkbox("Stop SDR When IDLE", &autotrack_cfg.stop_sdr_when_idle);
        elev_width += ImGui::GetItemRectSize().x - ImGui::CalcTextSize("Minimum Elevation").x - imgui_style.ItemInnerSpacing.x;
        ImGui::Checkbox("Display Local Time", &autotrack_cfg.use_localtime);
        ImGui::SetItemTooltip("%s", "Displays times as local time throughout the scheduler UI only.\nAll other times are in UTC.");
        ImGui::SetNextItemWidth(elev_width);
        ImGui::InputFloat("Minimum Elevation", &autotrack_cfg.autotrack_min_elevation);
        ImGui::EndGroup();
        ImGui::SetCursorPosY(selection_box_bottom);

        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("Update Passes"))
        {
            updateAutotrackPasses(curr_time);
        }
        if (autotrack_engaged)
            style::endDisabled();
        ImGui::SameLine();
        bool set_engaged = ImGui::Checkbox("Engage Autotrack", &autotrack_engaged);

#if 1
        {
            ImGui::Spacing();
            int d_pplot_height = (enabled_satellites.size() * 20) * ui_scale;
            int d_pplot_size = ImGui::GetWindowContentRegionMax().x;
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + d_pplot_size, ImGui::GetCursorScreenPos().y + d_pplot_height + 20 * ui_scale),
                                     style::theme.widget_bg);

            time_t tttime = curr_time;
            std::tm *timeReadable = (autotrack_cfg.use_localtime ? localtime(&tttime) : gmtime(&tttime));
            int curr_hour = timeReadable->tm_hour;
            int offset = d_pplot_size / 12 * (timeReadable->tm_min / 60.0);
            ImGui::Dummy(ImVec2(0, 0));
            for (int i = (timeReadable->tm_min < 30 ? 1 : 0); i < (timeReadable->tm_min < 30 ? 12 : 13); i++)
            {
                ImGui::SameLine();
                ImGui::SetCursorPosX(i * d_pplot_size / 12 - offset);
                ImGui::Text("%s%s%s", (curr_hour + i) % 24 < 10 ? "0" : "", std::to_string((curr_hour + i) % 24).c_str(), ":00");
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
                        double cpass_xs = ((cpass.aos_time - curr_time) / (12.0 * 3600.0)) * d_pplot_size;
                        double cpass_xe = ((cpass.los_time - curr_time) / (12.0 * 3600.0)) * d_pplot_size;

                        std::string name = "NORAD " + norad;
                        std::optional<TLE> this_tle = db_tle->get_from_norad(norad);
                        if (this_tle.has_value())
                            name = this_tle->name;

                        if (cpass_xs < 0)
                            cpass_xs = 0;
                        if (cpass_xe > d_pplot_size)
                            cpass_xe = d_pplot_size;
                        if (cpass_xs > d_pplot_size)
                            cpass_xs = d_pplot_size;

                        auto color = ImColor::HSV(fmod(norad, 10) / 10.0, 1, 1);
                        draw_list->AddRectFilled(ImVec2(ImGui::GetCursorScreenPos().x + cpass_xs, ImGui::GetCursorScreenPos().y + thsat_ys + 1),
                                                 ImVec2(ImGui::GetCursorScreenPos().x + cpass_xe, ImGui::GetCursorScreenPos().y + thsat_ye), color, 3);

                        if (first_pass)
                        {
                            draw_list->AddText(ImVec2(ImGui::GetCursorScreenPos().x + cpass_xe + 5 * ui_scale, ImGui::GetCursorScreenPos().y + thsat_ys + (sat_blk_height / 2) - 8 * ui_scale), color,
                                               name.c_str());
                            first_pass = false;
                        }

                        if (ImGui::IsMouseHoveringRect(ImVec2(ImGui::GetCursorScreenPos().x + cpass_xs, ImGui::GetCursorScreenPos().y + thsat_ys),
                                                       ImVec2(ImGui::GetCursorScreenPos().x + cpass_xe, ImGui::GetCursorScreenPos().y + thsat_ye)))
                            ImGui::SetTooltip("%s\nAOS : %s\nLOS : %s\nEl : %.2f", name.c_str(), timestamp_to_string(cpass.aos_time, autotrack_cfg.use_localtime).c_str(),
                                              timestamp_to_string(cpass.los_time, autotrack_cfg.use_localtime).c_str(), cpass.max_elevation);
                    }
                }

                draw_list->AddLine(ImVec2(ImGui::GetCursorScreenPos().x + 0, ImGui::GetCursorScreenPos().y + thsat_ys),
                                   ImVec2(ImGui::GetCursorScreenPos().x + d_pplot_size, ImGui::GetCursorScreenPos().y + thsat_ys), ImColor(100, 100, 100, 255));
                draw_list->AddLine(ImVec2(ImGui::GetCursorScreenPos().x + 0, ImGui::GetCursorScreenPos().y + thsat_ye),
                                   ImVec2(ImGui::GetCursorScreenPos().x + d_pplot_size, ImGui::GetCursorScreenPos().y + thsat_ye), ImColor(100, 100, 100, 255));

                if (autotrack_engaged)
                {
                    for (auto &cpass : upcoming_satellite_passes_sel)
                    {
                        if (cpass.norad == norad)
                        {
                            double cpass_xs = ((cpass.aos_time - curr_time) / (12.0 * 3600.0)) * d_pplot_size;
                            double cpass_xe = ((cpass.los_time - curr_time) / (12.0 * 3600.0)) * d_pplot_size;

                            if (cpass_xs < 0)
                                cpass_xs = 0;
                            if (cpass_xe > d_pplot_size)
                                cpass_xe = d_pplot_size;
                            if (cpass_xs > d_pplot_size)
                                cpass_xs = d_pplot_size;

                            auto color = ImGui::ColorConvertFloat4ToU32(imgui_style.Colors[ImGuiCol_Text]);
                            draw_list->AddRect(ImVec2(ImGui::GetCursorScreenPos().x + cpass_xs, ImGui::GetCursorScreenPos().y + thsat_ys),
                                               ImVec2(ImGui::GetCursorScreenPos().x + cpass_xe, ImGui::GetCursorScreenPos().y + thsat_ye), color, 3, 0, 2 * ui_scale);
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
            int sat_row = 0, new_hovered = -1;
            bool is_hovered = false;
            ImVec2 min_el_size(ImGui::CalcTextSize("Min El.").x + imgui_style.ItemInnerSpacing.x + (60.0f * ui_scale), 0.0f);
            float downlink_dd_width = (300 * ui_scale) - ImGui::CalcTextSize("Config").x - imgui_style.FramePadding.x * 2 - imgui_style.ItemSpacing.x;
            for (auto &cpass : enabled_satellites)
            {
                int dl_pos = 0;
                std::optional<satdump::TLE> thisTLE = db_tle->get_from_norad(cpass.norad);
                std::string object_name = (thisTLE.has_value() ? thisTLE->name : "NORAD #" + std::to_string(cpass.norad));
                for (auto &downlink : cpass.downlinks)
                {
                    std::string idpart = std::to_string(cpass.norad) + "_" + std::to_string((size_t)&downlink);
                    auto color = ImColor::HSV(fmod(cpass.norad, 10) / 10.0, 1, 1);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (&downlink == &cpass.downlinks[0])
                    {
                        ImGui::SetNextItemWidth(100 * ui_scale);
                        ImGui::TextColored(color, "%s", object_name.c_str());
                        if (ImGui::Button(((std::string) "+##objadddownlink" + std::to_string(cpass.norad)).c_str()))
                        {
                            cpass.downlinks.push_back(satdump::TrackedObject::Downlink());
                            break;
                        }
                    }
                    else
                    {
                        if (ImGui::Button(((std::string) "-##objdeldownlink" + idpart).c_str()))
                        {
                            cpass.downlinks.erase(cpass.downlinks.begin() + dl_pos);
                            break;
                        }
                    }

                    if (&downlink == &cpass.downlinks[0] && (sat_row == hovered_sat || cpass.min_elevation > 0))
                    {
                        ImGui::SetNextItemWidth(60 * ui_scale);
                        ImGui::InputFloat(((std::string) "Min El.##objdelminelevation" + idpart).c_str(), &cpass.min_elevation);
                        if (ImGui::IsItemActive())
                        {
                            is_hovered = true;
                            new_hovered = sat_row;
                        }
                        if (cpass.min_elevation < 0)
                            cpass.min_elevation = 0;
                        if (cpass.min_elevation > 90)
                            cpass.min_elevation = 90;
                    }
                    else
                        ImGui::Dummy(min_el_size);

                    std::string modal_title = "Configure " + object_name + " - " + downlink.pipeline_selector->selected_pipeline.name + " - " + format_notated(downlink.frequency, "Hz");

                    ImGui::TableSetColumnIndex(1);
                    widgets::FrequencyInput(((std::string) "Hz##objcfgfreq1" + idpart).c_str(), &downlink.frequency, 0.75f, false);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::SetNextItemWidth(100 * ui_scale);
                    ImGui::Checkbox(((std::string) "Record##objcfgfreq2" + idpart).c_str(), &downlink.record);
                    // ImGui::TableSetColumnIndex(3);
                    ImGui::Checkbox(((std::string) "Live##objcfgfreq3" + idpart).c_str(), &downlink.live);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::PushID(cpass.norad);

                    if (!downlink.record)
                        ImGui::BeginDisabled();
                    if (autotrack_cfg.multi_mode)
                        ImGui::SetNextItemWidth(100 * ui_scale);
                    else
                        ImGui::SetNextItemWidth(downlink_dd_width);
                    downlink.baseband_format.draw_record_combo();
                    if (autotrack_cfg.multi_mode)
                    {
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(100 * ui_scale);
                        if (ImGui::InputInt(((std::string) "Decim##recorddecim" + idpart).c_str(), &downlink.baseband_decimation) && downlink.baseband_decimation < 1)
                            downlink.baseband_decimation = 1;
                        ImGui::SetItemTooltip("IQ Decimation");
                    }
                    if (!downlink.record)
                        ImGui::EndDisabled();

                    if (!downlink.live)
                        ImGui::BeginDisabled();
                    ImGui::SetNextItemWidth(downlink_dd_width);
                    if (ImGui::BeginCombo(((std::string) "##pipelinesel" + idpart).c_str(), downlink.pipeline_selector->selected_pipeline.name.c_str(), ImGuiComboFlags_HeightLarge))
                    {
                        downlink.pipeline_selector->renderSelectionBox(300 * ui_scale);
                        ImGui::EndCombo();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button(((std::string) "Config##" + idpart).c_str()))
                        ImGui::OpenPopup(modal_title.c_str());
                    if (!downlink.live)
                        ImGui::EndDisabled();

                    if (ImGui::BeginPopupModal(modal_title.c_str()))
                    {
                        downlink.pipeline_selector->renderParamTable();
                        ImGui::SetCursorPosX((ImGui::GetContentRegionMax().x / 2) - (ImGui::CalcTextSize("Save").x + imgui_style.FramePadding.x * 2) / 2);
                        if (ImGui::Button("Save"))
                            ImGui::CloseCurrentPopup();
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();
                    // ImGui::InputText(((std::string) "Pipeline##objcfgfreq4" + std::to_string(cpass.norad)).c_str(), &cpass.pipeline_name);

                    ImRect namecell_rect = ImGui::TableGetCellBgRect(ImGui::GetCurrentContext()->CurrentTable, 0);
                    if (!is_hovered && !autotrack_engaged && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                        ImGui::IsMouseHoveringRect(namecell_rect.Min, namecell_rect.Max, false))
                    {
                        is_hovered = true;
                        new_hovered = sat_row;
                    }
                    dl_pos++;
                    sat_row++;
                }
            }
            if (!sat_was_hovered || is_hovered)
                hovered_sat = new_hovered;
            sat_was_hovered = is_hovered;

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
                ImGui::Text("%s", tle_registry->get_from_norad(cpass.norad)->name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f", cpass.max_elevation);
            }

            ImGui::EndTable();
        }
#endif

        upcoming_satellite_passes_mtx.unlock();
        if (set_engaged)
            setEngaged(autotrack_engaged, curr_time);

        ImGui::Dummy({0, 0});
    }

    image::Image AutoTrackScheduler::getScheduleImage(int width, double curr_time)
    {
        image::Image img;

        float ui_scale = 1;

        {
            int d_pplot_height = (enabled_satellites.size() * 20) * ui_scale;
            int d_pplot_size = width;

            img.init(8, width, d_pplot_height, 3);
            img.fill(0);
            image::TextDrawer text_drawer;
            text_drawer.init_font(resources::getResourcePath("fonts/font.ttf"));

            std::vector<double> color_gray = {100.0 / 255.0, 100.0 / 255.0, 100.0 / 255.0};
            std::vector<double> color_white = {1, 1, 1};

            time_t tttime = curr_time;
            std::tm *timeReadable = (autotrack_cfg.use_localtime ? localtime(&tttime) : gmtime(&tttime));
            int curr_hour = timeReadable->tm_hour;
            int offset = d_pplot_size / 12 * (timeReadable->tm_min / 60.0);
            //              ImGui::Dummy(ImVec2(0, 0));
            for (int i = (timeReadable->tm_min < 30 ? 1 : 0); i < (timeReadable->tm_min < 30 ? 12 : 13); i++)
            {
                text_drawer.draw_text(img, i * d_pplot_size / 12 - offset, 0, color_gray, 10,
                                      satdump::svformat("%s%s%s", (curr_hour + i) % 24 < 10 ? "0" : "", std::to_string((curr_hour + i) % 24).c_str(), ":00"));
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
                        double cpass_xs = ((cpass.aos_time - curr_time) / (12.0 * 3600.0)) * d_pplot_size;
                        double cpass_xe = ((cpass.los_time - curr_time) / (12.0 * 3600.0)) * d_pplot_size;

                        std::string name = db_tle->get_from_norad(norad)->name;

                        if (cpass_xs < 0)
                            cpass_xs = 0;
                        if (cpass_xe > d_pplot_size)
                            cpass_xe = d_pplot_size;
                        if (cpass_xs > d_pplot_size)
                            cpass_xs = d_pplot_size;

                        uint8_t color[3];
                        hsv_to_rgb(fmod(norad, 10) / 10.0, 1, 1, color);
                        img.draw_rectangle(cpass_xs, thsat_ys, cpass_xe, thsat_ye, {color[0] / 255.0, color[1] / 255.0, color[2] / 255.0}, true);

                        if (first_pass)
                        {
                            text_drawer.draw_text(img, cpass_xe + 5 * ui_scale, thsat_ys + (sat_blk_height / 2) - 6 * ui_scale, {color[0] / 255.0, color[1] / 255.0, color[2] / 255.0}, 10, name);
                            first_pass = false;
                        }
                    }
                }

                img.draw_line(0, thsat_ys, d_pplot_size - 1, thsat_ys, color_gray);
                img.draw_line(0, thsat_ye, d_pplot_size - 1, thsat_ye, color_gray);

                if (autotrack_engaged)
                {
                    for (auto &cpass : upcoming_satellite_passes_sel)
                    {
                        if (cpass.norad == norad)
                        {
                            double cpass_xs = ((cpass.aos_time - curr_time) / (12.0 * 3600.0)) * d_pplot_size;
                            double cpass_xe = ((cpass.los_time - curr_time) / (12.0 * 3600.0)) * d_pplot_size;

                            if (cpass_xs < 0)
                                cpass_xs = 0;
                            if (cpass_xe > d_pplot_size)
                                cpass_xe = d_pplot_size;
                            if (cpass_xs > d_pplot_size)
                                cpass_xs = d_pplot_size;

                            img.draw_rectangle(cpass_xs, thsat_ys, cpass_xe, thsat_ye - 1, color_white, false);
                        }
                    }
                }
            }
        }

        return img;
    }
} // namespace satdump
