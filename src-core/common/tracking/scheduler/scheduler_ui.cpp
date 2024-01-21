#include "scheduler.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "core/style.h"
#include "common/utils.h"
#include "common/widgets/frequency_input.h"

namespace satdump
{
    void AutoTrackScheduler::renderAutotrackConfig(bool light_theme, double curr_time)
    {
        if (!has_tle)
            return;

        upcoming_satellite_passes_mtx.lock();

        if (autotrack_engaged)
            style::beginDisabled();

        float curpos = ImGui::GetCursorPosY();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 220 * ui_scale);
        ImGui::SetNextItemWidth(200 * ui_scale);
        ImGui::InputTextWithHint("##trackingavailablesatssearch", u8"\uf422   Search All Satellites", &availablesatssearch);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 220 * ui_scale);
        ImGui::BeginGroup();
        ImGui::SetNextItemWidth(200 * ui_scale);
        if (ImGui::BeginListBox("##trackingavailablesatsbox"))
        {
            for (int i = 0; i < (int)satoptions.size(); i++)
                if (std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [i](TrackedObject &c)
                                 { return c.norad == general_tle_registry[i].norad; }) == enabled_satellites.end())
                    if (availablesatssearch.size() == 0 || isStringPresent(satoptions[i], availablesatssearch))
                    {
                        if (ImGui::Selectable(satoptions[i].c_str(), i == tracking_sats_menu_selected_1))
                            tracking_sats_menu_selected_1 = i;

                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                        {
                            auto it = std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [this, i](TrackedObject &t)
                                                   { return t.norad == general_tle_registry[i].norad; });
                            if (it == enabled_satellites.end())
                                enabled_satellites.push_back({general_tle_registry[i].norad});
                        }
                    }
            ImGui::EndListBox();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 15 * ui_scale);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50 * ui_scale);
        ImGui::BeginGroup();
        if (ImGui::Button(">>>"))
        {
            auto it = std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [this](TrackedObject &t)
                                   { return t.norad == general_tle_registry[tracking_sats_menu_selected_1].norad; });
            if (it == enabled_satellites.end())
                enabled_satellites.push_back({general_tle_registry[tracking_sats_menu_selected_1].norad});
        }
        if (ImGui::Button("<<<"))
        {
            auto it = std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [this](TrackedObject &t)
                                   { return t.norad == general_tle_registry[tracking_sats_menu_selected_2].norad; });
            if (it != enabled_satellites.end())
                enabled_satellites.erase(it);
        }
        ImGui::EndGroup();
        ImGui::SetCursorPosY(curpos);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 + 20 * ui_scale);
        ImGui::SetNextItemWidth(200 * ui_scale);
        ImGui::InputTextWithHint("##trackingselectedsatssearch", u8"\uf422   Search Selected", &selectedsatssearch);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 + 20 * ui_scale);
        ImGui::SetNextItemWidth(200 * ui_scale);
        if (ImGui::BeginListBox("##trackingselectedsatsbox"))
        {
            for (int i = 0; i < (int)satoptions.size(); i++)
                if (std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [i](TrackedObject &c)
                                 { return c.norad == general_tle_registry[i].norad; }) != enabled_satellites.end())
                    if (selectedsatssearch.size() == 0 || isStringPresent(satoptions[i], selectedsatssearch))
                    {
                        if (ImGui::Selectable(satoptions[i].c_str(), i == tracking_sats_menu_selected_2))
                            tracking_sats_menu_selected_2 = i;

                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                        {
                            auto it = std::find_if(enabled_satellites.begin(), enabled_satellites.end(), [this, i](TrackedObject &t)
                                                   { return t.norad == general_tle_registry[i].norad; });
                            if (it != enabled_satellites.end())
                                enabled_satellites.erase(it);
                        }
                    }

            ImGui::EndListBox();
        }
        ImGui::EndGroup();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::SetNextItemWidth(150 * ui_scale);
        ImGui::Checkbox("VFO Mode", &autotrack_cfg.vfo_mode);
        ImGui::InputFloat("Minimum Elevation", &autotrack_cfg.autotrack_min_elevation);
        ImGui::Checkbox("Stop SDR When IDLE", &autotrack_cfg.stop_sdr_when_idle);
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
            draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                     ImVec2(ImGui::GetCursorScreenPos().x + d_pplot_size, ImGui::GetCursorScreenPos().y + d_pplot_height + 20 * ui_scale),
                                     light_theme ? ImColor(255, 255, 255, 255) : ImColor::HSV(0, 0, 0));

            time_t tttime = curr_time;
            std::tm *timeReadable = gmtime(&tttime);
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
                            double cpass_xs = ((cpass.aos_time - curr_time) / (12.0 * 3600.0)) * d_pplot_size;
                            double cpass_xe = ((cpass.los_time - curr_time) / (12.0 * 3600.0)) * d_pplot_size;

                            if (cpass_xs < 0)
                                cpass_xs = 0;
                            if (cpass_xe > d_pplot_size)
                                cpass_xe = d_pplot_size;
                            if (cpass_xs > d_pplot_size)
                                cpass_xs = d_pplot_size;

                            auto color = ImColor(255, 255, 255, 255);
                            draw_list->AddRect(ImVec2(ImGui::GetCursorScreenPos().x + cpass_xs, ImGui::GetCursorScreenPos().y + thsat_ys),
                                               ImVec2(ImGui::GetCursorScreenPos().x + cpass_xe, ImGui::GetCursorScreenPos().y + thsat_ye),
                                               color, 3, 0, 2 * ui_scale);
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
                int dl_pos = 0;
                for (auto &downlink : cpass.downlinks)
                {
                    std::string idpart = std::to_string(cpass.norad) + "_" + std::to_string((size_t)&downlink);

                    auto color = ImColor::HSV(fmod(cpass.norad, 10) / 10.0, 1, 1);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (&downlink == &cpass.downlinks[0])
                    {
                        ImGui::SetNextItemWidth(100 * ui_scale);
                        ImGui::TextColored(color, "%s", general_tle_registry.get_from_norad(cpass.norad)->name.c_str());
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
                    ImGui::TableSetColumnIndex(1);
                    widgets::FrequencyInput(((std::string) "Hz##objcfgfreq1" + idpart).c_str(), &downlink.frequency, 0.75f);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::SetNextItemWidth(100 * ui_scale);
                    ImGui::Checkbox(((std::string) "Record##objcfgfreq2" + idpart).c_str(), &downlink.record);
                    // ImGui::TableSetColumnIndex(3);
                    ImGui::Checkbox(((std::string) "Live##objcfgfreq3" + idpart).c_str(), &downlink.live);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::SetNextItemWidth(300 * ui_scale);
                    ImGui::PushID(cpass.norad);
                    if (ImGui::BeginCombo(((std::string) "##pipelinesel" + idpart).c_str(), downlink.pipeline_selector->get_name(downlink.pipeline_selector->pipeline_id).c_str(), ImGuiComboFlags_HeightLarge))
                    {
                        downlink.pipeline_selector->renderSelectionBox(300 * ui_scale);
                        ImGui::EndCombo();
                    }
                    if (ImGui::BeginCombo(((std::string) "##params" + idpart).c_str(), "Configure..."))
                    {
                        downlink.pipeline_selector->renderParamTable();
                        ImGui::EndCombo();
                    }
                    if (downlink.record)
                    {
                        ImGui::SameLine();
                        ImGui::InputInt(((std::string) "##recorddecim" + idpart).c_str(), &downlink.baseband_decimation);
                    }
                    ImGui::PopID();
                    // ImGui::InputText(((std::string) "Pipeline##objcfgfreq4" + std::to_string(cpass.norad)).c_str(), &cpass.pipeline_name);
                    dl_pos++;
                }
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
        if (set_engaged)
            setEngaged(autotrack_engaged, curr_time);
    }
}