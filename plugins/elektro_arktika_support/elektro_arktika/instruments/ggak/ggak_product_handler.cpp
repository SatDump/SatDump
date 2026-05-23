#include "ggak_product_handler.h"
#include "core/style.h"
#include "imgui/implot/implot.h"
#include "logger.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace elektro_arktika
{
    namespace ggak
    {
        namespace
        {
            void insert_gap_markers(GGAKChannel &ch)
            {
                if (ch.timestamps.size() < 2)
                    return;

                std::vector<double> deltas;
                deltas.reserve(ch.timestamps.size());
                for (size_t i = 1; i < ch.timestamps.size(); i++)
                {
                    double dt = ch.timestamps[i] - ch.timestamps[i - 1];
                    if (dt > 0.01)
                        deltas.push_back(dt);
                }
                if (deltas.empty())
                    return;

                std::sort(deltas.begin(), deltas.end());
                double cadence = deltas[deltas.size() / 2];
                double gap_threshold = 1.5 * cadence;

                std::vector<double> new_ts, new_vals;
                new_ts.reserve(ch.timestamps.size() + ch.timestamps.size() / 8);
                new_vals.reserve(new_ts.capacity());

                new_ts.push_back(ch.timestamps[0]);
                new_vals.push_back(ch.values[0]);

                for (size_t i = 1; i < ch.timestamps.size(); i++)
                {
                    double dt = ch.timestamps[i] - ch.timestamps[i - 1];
                    if (dt > gap_threshold)
                    {
                        double nan = std::numeric_limits<double>::quiet_NaN();
                        new_ts.push_back(nan);
                        new_vals.push_back(nan);
                    }
                    new_ts.push_back(ch.timestamps[i]);
                    new_vals.push_back(ch.values[i]);
                }

                ch.timestamps = std::move(new_ts);
                ch.values = std::move(new_vals);
            }
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

            for (int i = 0; i < (int)ggak_product->subpacket_channels.size(); i++)
            {
                auto &ch = ggak_product->subpacket_channels[i];
                std::string grp = ch.group.empty() ? ch.name : ch.group;
                auto it = std::find_if(subpacket_group_info.begin(), subpacket_group_info.end(),
                                       [&](const SubpacketGroupInfo &g) { return g.name == grp; });
                if (it != subpacket_group_info.end())
                    it->channel_indices.push_back(i);
                else
                    subpacket_group_info.push_back({grp, {i}});
            }

            for (int i = 0; i < (int)ggak_product->particle_channels.size(); i++)
                particle_labels.push_back(ggak_product->particle_channels[i].name + "##ptcl_" + std::to_string(i));
            for (int i = 0; i < (int)subpacket_group_info.size(); i++)
                subpacket_rb_labels.push_back(subpacket_group_info[i].name + "##spkt_" + std::to_string(i));

            for (auto &ch : ggak_product->mag_channels)
                insert_gap_markers(ch);
            for (auto &ch : ggak_product->particle_channels)
                insert_gap_markers(ch);
            for (auto &ch : ggak_product->subpacket_channels)
                insert_gap_markers(ch);

            tryApplyDefaultPreset();
        }

        GGAKProductHandler::~GGAKProductHandler() {}

        void GGAKProductHandler::drawMenu()
        {
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
                    for (int i = 0; i < (int)ggak_product->particle_channels.size(); i++)
                    {
                        bool vis = particle_visible[i] != 0;
                        if (ImGui::Checkbox(particle_labels[i].c_str(), &vis))
                            particle_visible[i] = vis ? 1 : 0;
                    }
                }
            }
            else if (current_view == VIEW_SUBPACKETS)
            {
                if (ImGui::CollapsingHeader("Spectrometer##spectrometer_ch", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (int i = 0; i < (int)subpacket_group_info.size(); i++)
                    {
                        if (ImGui::RadioButton(subpacket_rb_labels[i].c_str(), selected_subpacket_group == i))
                            selected_subpacket_group = i;
                    }
                }
            }

        }

        void GGAKProductHandler::drawContents(ImVec2 win_size)
        {
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
            int total_s = (int)value;
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

        void GGAKProductHandler::drawChannelPlot(const GGAKChannel &ch, ImVec2 size, const char *y_label)
        {
            if (ch.values.empty())
                return;

            char title_buf[128];
            snprintf(title_buf, sizeof(title_buf), "%s (%s)##plot_%p",
                     ch.name.c_str(), ch.unit.c_str(), (const void *)&ch);
            if (ImPlot::BeginPlot(title_buf, size))
            {
                ImPlot::SetupAxes("Elapsed", y_label ? y_label : ch.unit.c_str(),
                                  ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisFormat(ImAxis_X1, ggak_time_formatter_cb);
                ImPlot::PlotLine(ch.name.c_str(),
                                 ch.timestamps.data(), ch.values.data(),
                                 (int)ch.values.size());
                ImPlot::EndPlot();
            }
        }

        void GGAKProductHandler::drawMagnetometerPlots(ImVec2 win_size)
        {
            if (ggak_product->mag_channels.empty())
            {
                ImGui::TextColored(style::theme.orange, "No magnetometer data available.");
                return;
            }

            bool *show_flags[] = {&mag_show_btotal, &mag_show_bx, &mag_show_by, &mag_show_bz};
            int mag_field_count = std::min(4, (int)ggak_product->mag_channels.size());

            int visible_count = 0;
            for (int i = 0; i < mag_field_count; i++)
                if (*show_flags[i])
                    visible_count++;

            bool has_voltage = mag_show_voltage && (int)ggak_product->mag_channels.size() > 4;

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
                        auto &ch = ggak_product->mag_channels[i];
                        if (ch.values.empty())
                            continue;
                        ImPlot::PlotLine(ch.name.c_str(),
                                         ch.timestamps.data(), ch.values.data(),
                                         (int)ch.values.size());
                    }
                    ImPlot::EndPlot();
                }

                if (has_voltage)
                    drawChannelPlot(ggak_product->mag_channels[4], ImVec2(win_size.x, per_aux_h), "V");
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
                    drawChannelPlot(ggak_product->mag_channels[i], ImVec2(win_size.x, plot_h), "nT");
                }

                if (has_voltage)
                    drawChannelPlot(ggak_product->mag_channels[4], ImVec2(win_size.x, plot_h), "V");
            }
        }

        void GGAKProductHandler::drawParticlePlots(ImVec2 win_size)
        {
            if (ggak_product->particle_channels.empty())
            {
                ImGui::TextColored(style::theme.orange, "No particle data available.");
                return;
            }

            int visible = 0;
            for (int i = 0; i < (int)particle_visible.size(); i++)
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

                    for (int i = 0; i < (int)ggak_product->particle_channels.size(); i++)
                    {
                        if (!particle_visible[i])
                            continue;
                        auto &ch = ggak_product->particle_channels[i];
                        if (ch.values.empty())
                            continue;
                        ImPlot::PlotLine(ch.name.c_str(),
                                         ch.timestamps.data(), ch.values.data(),
                                         (int)ch.values.size());
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

                for (int i = 0; i < (int)ggak_product->particle_channels.size(); i++)
                {
                    if (!particle_visible[i])
                        continue;
                    drawChannelPlot(ggak_product->particle_channels[i],
                                    ImVec2(win_size.x - 16, plot_h), "Counts");
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

            if (selected_subpacket_group < 0 || selected_subpacket_group >= (int)subpacket_group_info.size())
            {
                ImGui::Text("Selected group has no data.");
                return;
            }

            auto &grp = subpacket_group_info[selected_subpacket_group];
            int n_channels = (int)grp.channel_indices.size();
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
                auto &ch = ggak_product->subpacket_channels[idx];
                if (ch.values.empty())
                    continue;
                char sp_title[128];
                snprintf(sp_title, sizeof(sp_title), "%s (%s)##subpkt_%d",
                         ch.name.c_str(), ch.unit.c_str(), idx);
                if (ImPlot::BeginPlot(sp_title, ImVec2(win_size.x - 16, plot_h)))
                {
                    ImPlot::SetupAxes("Elapsed", ch.unit.c_str(),
                                      ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxisFormat(ImAxis_X1, ggak_time_formatter_cb);
                    ImPlot::PlotLine(ch.name.c_str(),
                                     ch.timestamps.data(), ch.values.data(),
                                     (int)ch.values.size());
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
                for (int i = (int)bt.values.size() - 1; i >= 0; i--)
                {
                    if (!std::isnan(bt.values[i]))
                    {
                        last_bt = bt.values[i];
                        break;
                    }
                }
                if (!std::isnan(last_bt))
                {
                    double dev = std::abs(last_bt - 103.0);
                    int level = dev < 5.0 ? 1 : dev < 20.0 ? 2 : dev < 50.0 ? 3 : dev < 100.0 ? 4 : 5;
                    const char *labels[] = {"Normal", "Slight", "Moderate", "Strong", "Extreme"};
                    ImVec4 colors[] = {
                        (ImVec4)style::theme.green, (ImVec4)style::theme.light_green,
                        (ImVec4)style::theme.yellow, (ImVec4)style::theme.orange,
                        (ImVec4)style::theme.red};
                    ImGui::Spacing();
                    ImGui::Text("Geomagnetic Activity:");
                    ImGui::SameLine();
                    ImGui::TextColored(colors[level - 1], "Level %d - %s", level, labels[level - 1]);
                    ImGui::Text("|B| = %.1f nT (baseline 103 nT, dev %.1f nT)", last_bt, dev);
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Channel overview table
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
                        ImGui::TextUnformatted(group);
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

                for (auto &ch : ggak_product->subpacket_channels)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
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

                ImGui::EndTable();
            }

        }
    } // namespace ggak
} // namespace elektro_arktika
