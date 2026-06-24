#include "ggak_product_handler.h"
#include "ggak_types.h"
#include "core/style.h"
#include "imgui/implot/implot.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace elektro_arktika
{
    namespace ggak
    {
        namespace
        {
            constexpr float SCROLLBAR_MARGIN = 16.0f;
        } // anonymous namespace

        GGAKProductHandler::GGAKProductHandler(std::shared_ptr<satdump::products::Product> p, bool dataset_mode)
            : ProductHandler(p, dataset_mode)
        {
            handler_tree_icon = u8"\uf076";
            ggak_product = dynamic_cast<GGAKProduct *>(ProductHandler::product.get());
            if (!ggak_product)
            {
                logger->error("GGAKProductHandler: product is not a GGAKProduct");
                return;
            }
            particle_visible.resize(ggak_product->particle_channels.size(), 1);

            plot_mag_channels = with_gap_markers(ggak_product->mag_channels);
            plot_particle_channels = with_gap_markers(ggak_product->particle_channels);
            plot_subpacket_channels = with_gap_markers(ggak_product->subpacket_channels);

            std::unordered_map<std::string, int> group_to_index;
            for (int i = 0; i < static_cast<int>(ggak_product->subpacket_channels.size()); i++)
            {
                auto &ch = ggak_product->subpacket_channels[i];
                std::string grp = ch.group.empty() ? ch.name : ch.group;
                auto it = group_to_index.find(grp);
                if (it != group_to_index.end())
                    subpacket_group_info[it->second].channel_indices.push_back(i);
                else
                {
                    group_to_index[grp] = static_cast<int>(subpacket_group_info.size());
                    subpacket_group_info.push_back({grp, {i}});
                }
            }

            tryApplyDefaultPreset();
        }

        GGAKProductHandler::~GGAKProductHandler() {}

        void GGAKProductHandler::drawMenu()
        {
            if (!ggak_product)
                return;

            if (ImGui::CollapsingHeader("View", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::RadioButton("Magnetometer", current_view == VIEW_MAGNETOMETER))
                    current_view = VIEW_MAGNETOMETER;
                if (ImGui::RadioButton("Particles", current_view == VIEW_PARTICLES))
                    current_view = VIEW_PARTICLES;
                if (ImGui::RadioButton("Spectrometer", current_view == VIEW_SUBPACKETS))
                    current_view = VIEW_SUBPACKETS;
                if (ImGui::RadioButton("Summary", current_view == VIEW_SUMMARY))
                    current_view = VIEW_SUMMARY;
            }

            ImGui::Spacing();

            if (current_view == VIEW_MAGNETOMETER)
            {
                if (ImGui::CollapsingHeader("FM-VE Channels", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Overlay all B##mag_overlay", &mag_overlay);
                    ImGui::Separator();
                    ImGui::Checkbox("|B| total##mag_bt", &mag_show_btotal);
                    ImGui::Checkbox("Bx##mag_bx", &mag_show_bx);
                    ImGui::Checkbox("By##mag_by", &mag_show_by);
                    ImGui::Checkbox("Bz##mag_bz", &mag_show_bz);
                    ImGui::Separator();
                    ImGui::Checkbox("Voltage##mag_voltage", &mag_show_voltage);
                }
            }
            else if (current_view == VIEW_PARTICLES)
            {
                if (ImGui::CollapsingHeader("GALS-VE Channels", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Overlay##ptcl_overlay", &particle_overlay);
                    ImGui::Separator();
                    for (int i = 0; i < static_cast<int>(ggak_product->particle_channels.size()); i++)
                    {
                        bool vis = particle_visible[i] != 0;
                        ImGui::PushID(i);
                        if (ImGui::Checkbox(ggak_product->particle_channels[i].name.c_str(), &vis))
                            particle_visible[i] = vis ? 1 : 0;
                        ImGui::PopID();
                    }
                }
            }
            else if (current_view == VIEW_SUBPACKETS)
            {
                if (ImGui::CollapsingHeader("Spectrometer##spectrometer_ch", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (int i = 0; i < static_cast<int>(subpacket_group_info.size()); i++)
                    {
                        ImGui::PushID(i);
                        if (ImGui::RadioButton(subpacket_group_info[i].name.c_str(), selected_subpacket_group == i))
                            selected_subpacket_group = i;
                        ImGui::PopID();
                    }
                }
            }

            renderPresetMenu();
        }

        void GGAKProductHandler::drawContents(ImVec2 win_size)
        {
            if (!ggak_product)
                return;

            plot_id_counter = 0;

            switch (current_view)
            {
            case VIEW_MAGNETOMETER:
                drawMagnetometerPlots(win_size);
                break;
            case VIEW_PARTICLES:
                drawParticlePlots(win_size);
                break;
            case VIEW_SUBPACKETS:
                drawSubpacketPlots(win_size);
                break;
            case VIEW_SUMMARY:
                drawSummary(win_size);
                break;
            }
        }

        static int ggak_time_formatter_cb(double value, char *buff, int size, void *)
        {
            if (value < 0.0)
                value = 0.0;
            else if (value > 2e9)
                value = 2e9;
            int total_s = static_cast<int>(value);
            int d = total_s / 86400;
            int h = (total_s % 86400) / 3600;
            int m = (total_s % 3600) / 60;
            int s = total_s % 60;
            if (d > 0)
                snprintf(buff, size, "D%d %02d:%02d", d, h, m);
            else
                snprintf(buff, size, "%02d:%02d:%02d", h, m, s);
            return 0;
        }

        void GGAKProductHandler::drawChannelPlot(const GGAKChannel &ch, ImVec2 size, int plot_idx, const char *y_label)
        {
            if (ch.values.empty())
                return;

            char title_buf[128];
            snprintf(title_buf, sizeof(title_buf), "%s (%s)##plot_%d",
                     ch.name.c_str(), ch.unit.c_str(), plot_idx);
            if (ImPlot::BeginPlot(title_buf, size))
            {
                ImPlot::SetupAxes("Elapsed", y_label ? y_label : ch.unit.c_str(),
                                  ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisFormat(ImAxis_X1, ggak_time_formatter_cb);
                ImPlot::PlotLine(ch.name.c_str(),
                                 ch.timestamps.data(), ch.values.data(),
                                 static_cast<int>(ch.values.size()));
                ImPlot::EndPlot();
            }
        }

        void GGAKProductHandler::drawMagnetometerPlots(ImVec2 win_size)
        {
            if (plot_mag_channels.empty())
            {
                ImGui::TextColored(style::theme.orange, "No magnetometer data available.");
                return;
            }

            constexpr int MAG_FIELD_COUNT = 4;
            std::array<bool*, MAG_FIELD_COUNT> show_flags = {
                &mag_show_btotal, &mag_show_bx, &mag_show_by, &mag_show_bz};
            int mag_field_count = std::min(MAG_FIELD_COUNT, static_cast<int>(plot_mag_channels.size()));

            int visible_count = 0;
            for (int i = 0; i < mag_field_count; i++)
                if (*show_flags[i])
                    visible_count++;

            bool has_voltage = mag_show_voltage && static_cast<int>(plot_mag_channels.size()) > 4;

            int aux_count = has_voltage ? 1 : 0;

            if (mag_overlay)
            {
                float aux_h = aux_count > 0 ? win_size.y * 0.20f * aux_count : 0;
                float main_h = win_size.y - aux_h;
                float per_aux_h = aux_count > 0 ? aux_h / aux_count : 0;

                if (ImPlot::BeginPlot("##FM-VE Magnetic Field", ImVec2(win_size.x, main_h)))
                {
                    ImPlot::SetupAxes("Elapsed", "nT", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxisFormat(ImAxis_X1, ggak_time_formatter_cb);

                    for (int i = 0; i < mag_field_count; i++)
                    {
                        if (!*show_flags[i])
                            continue;
                        auto &ch = plot_mag_channels[i];
                        if (ch.values.empty())
                            continue;
                        ImPlot::PlotLine(ch.name.c_str(),
                                         ch.timestamps.data(), ch.values.data(),
                                         static_cast<int>(ch.values.size()));
                    }
                    ImPlot::EndPlot();
                }

                if (has_voltage)
                    drawChannelPlot(plot_mag_channels[4], ImVec2(win_size.x, per_aux_h), plot_id_counter++, "V");
            }
            else
            {
                int n_plots = visible_count + aux_count;
                if (n_plots == 0)
                    n_plots = 1;

                float plot_h = win_size.y / n_plots;

                for (int i = 0; i < mag_field_count; i++)
                {
                    if (!*show_flags[i])
                        continue;
                    drawChannelPlot(plot_mag_channels[i], ImVec2(win_size.x, plot_h), plot_id_counter++, "nT");
                }

                if (has_voltage)
                    drawChannelPlot(plot_mag_channels[4], ImVec2(win_size.x, plot_h), plot_id_counter++, "V");
            }
        }

        void GGAKProductHandler::drawParticlePlots(ImVec2 win_size)
        {
            if (plot_particle_channels.empty())
            {
                ImGui::TextColored(style::theme.orange, "No particle data available.");
                return;
            }

            int visible = 0;
            for (int i = 0; i < static_cast<int>(particle_visible.size()); i++)
                if (particle_visible[i])
                    visible++;

            if (visible == 0)
            {
                ImGui::Text("Select at least one channel.");
                return;
            }

            if (particle_overlay)
            {
                if (ImPlot::BeginPlot("##GALS-VE Particle Counts", ImVec2(win_size.x, win_size.y)))
                {
                    ImPlot::SetupAxes("Elapsed", "Counts",
                                      ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxisFormat(ImAxis_X1, ggak_time_formatter_cb);

                    for (int i = 0; i < static_cast<int>(plot_particle_channels.size()); i++)
                    {
                        if (!particle_visible[i])
                            continue;
                        auto &ch = plot_particle_channels[i];
                        if (ch.values.empty())
                            continue;
                        ImPlot::PlotLine(ch.name.c_str(),
                                         ch.timestamps.data(), ch.values.data(),
                                         static_cast<int>(ch.values.size()));
                    }
                    ImPlot::EndPlot();
                }
            }
            else
            {
                float min_plot_h = ImGui::GetTextLineHeight() * 12;
                float plot_h = std::max(min_plot_h, win_size.y / visible);
                float total_h = plot_h * visible;

                if (total_h > win_size.y)
                    ImGui::BeginChild("##ptcl_scroll", win_size, false, ImGuiWindowFlags_NoBackground);

                for (int i = 0; i < static_cast<int>(plot_particle_channels.size()); i++)
                {
                    if (!particle_visible[i])
                        continue;
                    drawChannelPlot(plot_particle_channels[i],
                                    ImVec2(win_size.x - SCROLLBAR_MARGIN, plot_h), plot_id_counter++, "Counts");
                }

                if (total_h > win_size.y)
                    ImGui::EndChild();
            }
        }

        void GGAKProductHandler::drawSubpacketPlots(ImVec2 win_size)
        {
            if (subpacket_group_info.empty())
            {
                ImGui::TextColored(style::theme.orange, "No spectrometer data available.");
                return;
            }

            if (selected_subpacket_group < 0 || selected_subpacket_group >= static_cast<int>(subpacket_group_info.size()))
            {
                ImGui::Text("Selected group has no data.");
                return;
            }

            auto &grp = subpacket_group_info[selected_subpacket_group];
            int n_channels = static_cast<int>(grp.channel_indices.size());
            if (n_channels == 0)
            {
                ImGui::Text("No data in this group.");
                return;
            }

            float min_plot_h = ImGui::GetTextLineHeight() * 12;
            float plot_h = std::max(min_plot_h, win_size.y / n_channels);
            float total_h = plot_h * n_channels;

            if (total_h > win_size.y)
                ImGui::BeginChild("##spkt_scroll", win_size, false, ImGuiWindowFlags_NoBackground);

            for (int idx : grp.channel_indices)
            {
                auto &ch = plot_subpacket_channels[idx];
                if (ch.values.empty())
                    continue;
                char sp_title[128];
                snprintf(sp_title, sizeof(sp_title), "%s (%s)##subpkt_%d",
                         ch.name.c_str(), ch.unit.c_str(), idx);
                if (ImPlot::BeginPlot(sp_title, ImVec2(win_size.x - SCROLLBAR_MARGIN, plot_h)))
                {
                    ImPlot::SetupAxes("Elapsed", ch.unit.c_str(),
                                      ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxisFormat(ImAxis_X1, ggak_time_formatter_cb);
                    ImPlot::PlotLine(ch.name.c_str(),
                                     ch.timestamps.data(), ch.values.data(),
                                     static_cast<int>(ch.values.size()));
                    ImPlot::EndPlot();
                }
            }

            if (total_h > win_size.y)
                ImGui::EndChild();
        }

        void GGAKProductHandler::drawSummary(ImVec2 win_size)
        {
            auto &s = ggak_product->stats;

            ImGui::TextColored(style::theme.cyan, "GGAK Space Environment Monitor");
            ImGui::Text("Generation: %s", s.generation.c_str());
            ImGui::Spacing();

            float checksum_ratio = (s.checksum_pass + s.checksum_fail) > 0
                                       ? (float)s.checksum_pass / (s.checksum_pass + s.checksum_fail)
                                       : 0;
            ImGui::Text("Checksum: ");
            ImGui::SameLine();
            ImVec4 csum_col = checksum_ratio > 0.99f   ? (ImVec4)style::theme.green
                              : checksum_ratio > 0.9f  ? (ImVec4)style::theme.yellow
                                                       : (ImVec4)style::theme.red;
            ImGui::TextColored(csum_col, "%d pass / %d fail (%.1f%%)",
                               s.checksum_pass, s.checksum_fail, checksum_ratio * 100.0f);

            if (!ggak_product->mag_channels.empty() && !ggak_product->mag_channels[0].values.empty())
            {
                auto &bt = ggak_product->mag_channels[0];
                double last_bt = std::numeric_limits<double>::quiet_NaN();
                for (int i = static_cast<int>(bt.values.size()) - 1; i >= 0; i--)
                {
                    if (!std::isnan(bt.values[i]))
                    {
                        last_bt = bt.values[i];
                        break;
                    }
                }
                if (!std::isnan(last_bt))
                {
                    double dev = std::abs(last_bt - MAG_BASELINE_NT);
                    int level = geomagnetic_activity_level(last_bt);
                    const char *label = geomagnetic_activity_label(level);
                    ImVec4 colors[] = {
                        (ImVec4)style::theme.green, (ImVec4)style::theme.light_green,
                        (ImVec4)style::theme.yellow, (ImVec4)style::theme.orange,
                        (ImVec4)style::theme.red};
                    int color_idx = std::max(0, std::min(level - 1, 4));
                    ImGui::Spacing();
                    ImGui::Text("Geomagnetic Activity:");
                    ImGui::SameLine();
                    ImGui::TextColored(colors[color_idx], "Level %d - %s", level, label);
                    ImGui::Text("|B| = %.1f nT (baseline %.0f nT, dev %.1f nT)",
                                last_bt, MAG_BASELINE_NT, dev);
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(style::theme.cyan, "Channel Overview");
            ImGui::Spacing();

            if (ImGui::BeginTable("##ggak_channels", 5,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
                                  ImVec2(0, win_size.y * 0.5f)))
            {
                ImGui::TableSetupColumn("Group");
                ImGui::TableSetupColumn("Channel");
                ImGui::TableSetupColumn("Unit");
                ImGui::TableSetupColumn("Samples");
                ImGui::TableSetupColumn("Range");
                ImGui::TableHeadersRow();
                ImGui::TableSetupScrollFreeze(0, 1);

                auto draw_channel_rows = [](const char *group, const std::vector<GGAKChannel> &channels) {
                    for (auto &ch : channels)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        if (group)
                            ImGui::TextUnformatted(group);
                        else
                            ImGui::TextUnformatted(ch.group.empty() ? ch.name.c_str() : ch.group.c_str());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(ch.name.c_str());
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(ch.unit.c_str());
                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%d", ch.cached_valid_count);
                        ImGui::TableSetColumnIndex(4);
                        if (ch.cached_valid_count > 0)
                            ImGui::Text("%.2f .. %.2f", ch.cached_min, ch.cached_max);
                        else
                            ImGui::TextDisabled("--");
                    }
                };

                draw_channel_rows("FM-VE", ggak_product->mag_channels);
                draw_channel_rows("GALS-VE + SKIF-VE MIP", ggak_product->particle_channels);
                draw_channel_rows(nullptr, ggak_product->subpacket_channels);

                ImGui::EndTable();
            }

        }

        void GGAKProductHandler::setConfig(nlohmann::json p)
        {
            if (p.contains("view"))
            {
                std::string v = p["view"].get<std::string>();
                if (v == "magnetometer")
                    current_view = VIEW_MAGNETOMETER;
                else if (v == "particles")
                    current_view = VIEW_PARTICLES;
                else if (v == "spectrometer")
                    current_view = VIEW_SUBPACKETS;
                else if (v == "summary")
                    current_view = VIEW_SUMMARY;
            }
            if (p.contains("mag_overlay"))
                mag_overlay = p["mag_overlay"].get<bool>();
            if (p.contains("particle_overlay"))
                particle_overlay = p["particle_overlay"].get<bool>();
            if (p.contains("selected_subpacket_group"))
                selected_subpacket_group = p["selected_subpacket_group"].get<int>();
            if (p.contains("mag_show_btotal"))
                mag_show_btotal = p["mag_show_btotal"].get<bool>();
            if (p.contains("mag_show_bx"))
                mag_show_bx = p["mag_show_bx"].get<bool>();
            if (p.contains("mag_show_by"))
                mag_show_by = p["mag_show_by"].get<bool>();
            if (p.contains("mag_show_bz"))
                mag_show_bz = p["mag_show_bz"].get<bool>();
            if (p.contains("mag_show_voltage"))
                mag_show_voltage = p["mag_show_voltage"].get<bool>();
            if (p.contains("particle_visible") && p["particle_visible"].is_array())
            {
                auto arr = p["particle_visible"];
                for (size_t i = 0; i < arr.size() && i < particle_visible.size(); i++)
                    particle_visible[i] = arr[i].get<int>();
            }
        }

        nlohmann::json GGAKProductHandler::getConfig()
        {
            nlohmann::json p;

            switch (current_view)
            {
            case VIEW_MAGNETOMETER: p["view"] = "magnetometer"; break;
            case VIEW_PARTICLES:    p["view"] = "particles"; break;
            case VIEW_SUBPACKETS:   p["view"] = "spectrometer"; break;
            case VIEW_SUMMARY:      p["view"] = "summary"; break;
            }

            p["mag_overlay"] = mag_overlay;
            p["mag_show_btotal"] = mag_show_btotal;
            p["mag_show_bx"] = mag_show_bx;
            p["mag_show_by"] = mag_show_by;
            p["mag_show_bz"] = mag_show_bz;
            p["mag_show_voltage"] = mag_show_voltage;
            p["particle_overlay"] = particle_overlay;
            p["particle_visible"] = particle_visible;
            p["selected_subpacket_group"] = selected_subpacket_group;

            return p;
        }
    } // namespace ggak
} // namespace elektro_arktika
