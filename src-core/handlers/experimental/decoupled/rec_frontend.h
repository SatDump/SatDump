#pragma once

#include "base/remote_handler.h"
#include "base/remote_handler_backend.h"
#include "common/widgets/fft_plot.h"
#include "common/widgets/menuitem_tooltip.h"
#include "common/widgets/waterfall_plot.h"
#include "core/resources.h"
#include "core/style.h"
#include "dsp/device/dev.h"
#include "dsp/device/options_displayer.h"
#include "dsp/io/iq_types.h"
#include "handlers/experimental/decoupled/fft_wat_widget/fft_waterfall.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "utils/task_queue.h"
#include <cstddef>
#include <mutex>
#include <volk/volk_alloc.hh>

namespace satdump
{
    namespace handlers
    {
        class RecFrontendHandler : public RemoteHandlerHandler
        {
        private:
            std::vector<ndsp::DeviceInfo> available_devices;
            ndsp::DeviceInfo current_device;

            ndsp::OptionsDisplayer dev_opt_disp;

            std::mutex fm;
            TaskQueue tq;

            volk::vector<float> fft_vec;
            // std::shared_ptr<widgets::FFTPlot> fft_plot;
            // std::shared_ptr<widgets::WaterfallPlot> waterfall_plot;

            widgets::FFTWaterfallWidget wip_fft_widget;

            bool is_started = false;

            size_t last_fft_size = 0;
            int fft_size = 0;
            int fft_rate = 0;
            int fft_avg_num = 0;

            float buffer_usage = 0;

            ndsp::IQType rec_type;
            bool recording = false;
            size_t rec_size = 0;

            bool show_waterfall = true;

        protected:
            bool mustUpdate = true;
            void handle_stream_data(std::string id, uint8_t *data, size_t size)
            {
                if (id == "upd")
                {
                    mustUpdate = true;
                }
                else if (id == "fft" && (size / sizeof(float)) >= 8)
                {
                    if (size != last_fft_size)
                    {
                        fft_vec.resize(size / sizeof(float));
                        wip_fft_widget.set_fft_ptr(fft_vec.data());        //          fft_plot->set_ptr(fft_vec.data());
                        wip_fft_widget.set_fft_size(size / sizeof(float)); //         fft_plot->set_size(size / sizeof(float));
                        last_fft_size = size;
                    }

                    memcpy(fft_vec.data(), data, size);
                    wip_fft_widget.push_waterfall_fft(fft_vec.data(), size / sizeof(float));
                }
                else if (id == "rec_size" && size == sizeof(size_t))
                {
                    rec_size = *((size_t *)data);
                }
                else if (id == "buffer_usage" && size == sizeof(float))
                {
                    buffer_usage = *((float *)data);
                }
            }

        public:
            RecFrontendHandler(std::shared_ptr<RemoteHandlerBackend> bkd)
                : RemoteHandlerHandler(bkd), //
                  wip_fft_widget(65536 * 4, 2000)
            {
                fft_vec.resize(8);

                wip_fft_widget.set_fft_ptr(fft_vec.data());
                wip_fft_widget.set_fft_size(8);

                // fft_plot = std::make_shared<widgets::FFTPlot>(fft_vec.data(), 8, -150, 150, 10);
                // fft_plot->frequency = 431.8e6;
                // fft_plot->enable_freq_scale = true;

                //  waterfall_plot = std::make_shared<widgets::WaterfallPlot>(65536 * 4, 2000);
                // wip_fft_widget.set_waterfall_size(8);
                wip_fft_widget.set_waterfall_rate(30, 20);
                wip_fft_widget.set_waterfall_palette(colormaps::loadMap(resources::getResourcePath("waterfall/classic.json")));

                wip_fft_widget.band_callback = [](auto v) { logger->critical("BAND %s %f", v.id.c_str(), v.b); };
                wip_fft_widget.freq_callback = [](auto v) { logger->critical("FREQ %s %f", v.id.c_str(), v.f); };
            }
            ~RecFrontendHandler() {}

            void resyncFFT()
            {
                if (wip_fft_widget.fft_scale_max < wip_fft_widget.fft_scale_min)
                {
                    wip_fft_widget.fft_scale_min = wip_fft_widget.waterfall_scale_min;
                    wip_fft_widget.fft_scale_max = wip_fft_widget.waterfall_scale_max;
                }
                else if (wip_fft_widget.fft_scale_min > wip_fft_widget.fft_scale_max)
                {
                    wip_fft_widget.fft_scale_min = wip_fft_widget.waterfall_scale_min;
                    wip_fft_widget.fft_scale_max = wip_fft_widget.waterfall_scale_max;
                }
                else
                {
                    wip_fft_widget.waterfall_scale_min = wip_fft_widget.fft_scale_min;
                    wip_fft_widget.waterfall_scale_max = wip_fft_widget.fft_scale_max;
                }
            }

            // The Rest
            void drawMenu()
            {
                if (mustUpdate)
                {
                    tq.push(
                        [this]()
                        {
                            std::scoped_lock l(fm);
                            available_devices = bkd->get_cfg("available_devices");
                            current_device = bkd->get_cfg("current_device");

                            dev_opt_disp.clear();
                            dev_opt_disp.add_options(bkd->get_cfg("dev/list"));
                            dev_opt_disp.set_values(bkd->get_cfg("dev/cfg"));

                            fft_size = bkd->get_cfg("fft/size");
                            fft_rate = bkd->get_cfg("fft/rate");
                            fft_avg_num = bkd->get_cfg("fft/avg_num");
                            resyncFFT();

                            is_started = bkd->get_cfg("started");

                            buffer_usage = bkd->get_cfg("buffer_usage");

                            recording = bkd->get_cfg("recording");
                            rec_size = bkd->get_cfg("rec_size");
                        });

                    mustUpdate = false;
                }

                // std::scoped_lock l(fm);

                if (ImGui::CollapsingHeader("Source", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::Button("Add Audio"))
                    {
                        tq.push(
                            [this]()
                            {
                                std::scoped_lock l(fm);
                                bkd->set_cfg("add_audio", 0);
                            });
                    }
#if 0
                    if (is_started)
                        style::beginDisabled();
                    if (ImGui::BeginCombo("##devicebox", current_device.name.c_str()))
                    {
                        for (auto &d : available_devices)
                        {
                            if (ImGui::Selectable(d.name.c_str(), d.name == current_device.name))
                            {
                                tq.push(
                                    [this, d]()
                                    {
                                        std::scoped_lock l(fm);
                                        bkd->set_cfg("current_device", (nlohmann::json)d);

                                        dev_opt_disp.clear();
                                        dev_opt_disp.add_options(bkd->get_cfg("dev/list"));
                                        dev_opt_disp.set_values(bkd->get_cfg("dev/cfg"));
                                    });
                            }
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Refresh"))
                    {
                        tq.push(
                            [this]()
                            {
                                std::scoped_lock l(fm);
                                bkd->set_cfg("refresh", true);
                            });
                    }
                    if (is_started)
                        style::endDisabled();

                    ImGui::Separator();
#endif

                    nlohmann::json changed = dev_opt_disp.draw();

                    if (changed.size() > 0)
                    {
                        tq.push(
                            [this, changed]()
                            {
                                std::scoped_lock l(fm);
                                auto r = bkd->set_cfg("dev/cfg", changed);
                                if (r >= RemoteHandlerBackend::RES_LISTUPD)
                                {
                                    dev_opt_disp.clear();
                                    dev_opt_disp.add_options(bkd->get_cfg("dev/list"));
                                    dev_opt_disp.set_values(bkd->get_cfg("dev/cfg"));
                                }
                            });
                    }
                }

                if (ImGui::CollapsingHeader("FFT", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    widgets::SteppedSliderFloat("FFT Max", &wip_fft_widget.fft_scale_max, -160, 150);
                    widgets::SteppedSliderFloat("FFT Min", &wip_fft_widget.fft_scale_min, -160, 150);
                    ImGui::Checkbox("Show Waterfall", &show_waterfall);

                    resyncFFT();

                    if (ImGui::InputInt("Size##fftsize", &fft_size))
                    {
                        tq.push(
                            [this]()
                            {
                                std::scoped_lock l(fm);
                                bkd->set_cfg("fft/size", fft_size);
                            });
                    }

                    if (ImGui::InputInt("Rate##fftrate", &fft_rate))
                        tq.push(
                            [this]()
                            {
                                std::scoped_lock l(fm);
                                bkd->set_cfg("fft/rate", fft_rate);
                            });

                    if (ImGui::InputInt("Avg Num##fftavg", &fft_avg_num))
                        tq.push(
                            [this]()
                            {
                                std::scoped_lock l(fm);
                                bkd->set_cfg("fft/avg_num", fft_avg_num);
                            });
                }

                if (ImGui::CollapsingHeader("Recording", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (!is_started)
                        style::beginDisabled();

                    if (recording)
                        style::beginDisabled();
                    if (rec_type.draw_combo())
                        tq.push(
                            [this]()
                            {
                                std::scoped_lock l(fm);
                                bkd->set_cfg("rec_type", rec_type);
                            });
                    if (recording)
                        style::endDisabled();

                    if (!recording)
                    {
                        if (ImGui::Button("Rec Start"))
                        {
                            tq.push(
                                [this]()
                                {
                                    std::scoped_lock l(fm);
                                    bkd->set_cfg("recording", true);
                                });
                        }
                    }
                    else
                    {
                        ImGui::Text("Size : %lu", rec_size);

                        if (ImGui::Button("Rec Stop"))
                        {
                            tq.push(
                                [this]()
                                {
                                    std::scoped_lock l(fm);
                                    bkd->set_cfg("recording", false);
                                });
                        }
                    }

                    if (!is_started)
                        style::endDisabled();
                }
            }

            void drawContents(ImVec2 win_size)
            {
                ImGui::BeginChild("RecorderFFT", win_size, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                wip_fft_widget.draw(win_size, show_waterfall);
                ImGui::EndChild();
            }

            void drawMenuBar()
            {
                if (ImGui::BeginMenu("D", !is_started))
                {
                    for (auto &d : available_devices)
                    {
                        if (ImGui::MenuItem(d.name.c_str(), NULL, d.name == current_device.name))
                        {
                            tq.push(
                                [this, d]()
                                {
                                    std::scoped_lock l(fm);
                                    bkd->set_cfg("current_device", (nlohmann::json)d);

                                    dev_opt_disp.clear();
                                    dev_opt_disp.add_options(bkd->get_cfg("dev/list"));
                                    dev_opt_disp.set_values(bkd->get_cfg("dev/cfg"));
                                });
                        }
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Refresh"))
                    {
                        tq.push(
                            [this]()
                            {
                                std::scoped_lock l(fm);
                                bkd->set_cfg("refresh", true);
                            });
                    }

                    ImGui::EndMenu();
                }

                if (!is_started)
                {
                    if (widgets::MenuItemTooltip(u8"\uf909", "Start source"))
                    {
                        tq.push(
                            [this]()
                            {
                                std::scoped_lock l(fm);
                                bkd->set_cfg("started", true);
                            });
                    }
                }
                else
                {
                    if (widgets::MenuItemTooltip(u8"\ufc62", "Stop source"))
                    {
                        tq.push(
                            [this]()
                            {
                                std::scoped_lock l(fm);
                                bkd->set_cfg("started", false);
                            });
                    }
                }

                ImGui::Spacing();
                ImGui::ProgressBar(buffer_usage);
            }

            std::string getName() { return "RemoteHandleTest"; }

            std::string getID() { return "remote_test_handler"; }
        };
    } // namespace handlers
} // namespace satdump