#pragma once

#include "base/remote_handler.h"
#include "base/remote_handler_backend.h"
#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"
#include "core/resources.h"
#include "core/style.h"
#include "dsp/device/dev.h"
#include "dsp/device/options_displayer.h"
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
            std::shared_ptr<widgets::FFTPlot> fft_plot;
            std::shared_ptr<widgets::WaterfallPlot> waterfall_plot;

            bool is_started = false;

            size_t last_fft_size = 0;
            int fft_size = 0;
            int fft_rate = 0;
            int fft_avg_num = 0;

            float buffer_usage = 0;

            bool recording = false;
            size_t rec_size = 0;

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
                        fft_plot->set_ptr(fft_vec.data());
                        fft_plot->set_size(size / sizeof(float));
                        waterfall_plot->set_size(size / sizeof(float));
                        last_fft_size = size;
                    }

                    memcpy(fft_vec.data(), data, size);
                    waterfall_plot->push_fft(fft_vec.data());
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
            RecFrontendHandler(std::shared_ptr<RemoteHandlerBackend> bkd) : RemoteHandlerHandler(bkd)
            {
                fft_vec.resize(8);

                fft_plot = std::make_shared<widgets::FFTPlot>(fft_vec.data(), 8, -150, 150, 10);
                fft_plot->frequency = 431.8e6;
                fft_plot->enable_freq_scale = true;

                waterfall_plot = std::make_shared<widgets::WaterfallPlot>(65536 * 4, 2000);
                waterfall_plot->set_size(8);
                waterfall_plot->set_rate(30, 20);
                waterfall_plot->set_palette(colormaps::loadMap(resources::getResourcePath("waterfall/classic.json")));
            }
            ~RecFrontendHandler() {}

            void resyncFFT()
            {
                if (fft_plot->scale_max < fft_plot->scale_min)
                {
                    fft_plot->scale_min = waterfall_plot->scale_min;
                    fft_plot->scale_max = waterfall_plot->scale_max;
                }
                else if (fft_plot->scale_min > fft_plot->scale_max)
                {
                    fft_plot->scale_min = waterfall_plot->scale_min;
                    fft_plot->scale_max = waterfall_plot->scale_max;
                }
                else
                {
                    waterfall_plot->scale_min = fft_plot->scale_min;
                    waterfall_plot->scale_max = fft_plot->scale_max;
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
                    if (is_started)
                        style::beginDisabled();
                    if (ImGui::BeginCombo("Device##devicebox", current_device.name.c_str()))
                    {
                        for (auto &d : available_devices)
                        {
                            if (ImGui::Selectable(d.name.c_str(), d == current_device))
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
                    if (is_started)
                        style::endDisabled();

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

                    if (!is_started)
                    {
                        if (ImGui::Button("Start"))
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
                        if (ImGui::Button("Stop"))
                        {
                            tq.push(
                                [this]()
                                {
                                    std::scoped_lock l(fm);
                                    bkd->set_cfg("started", false);
                                });
                        }
                    }

                    ImGui::ProgressBar(buffer_usage);
                }

                if (ImGui::CollapsingHeader("FFT", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    widgets::SteppedSliderFloat("FFT Max", &fft_plot->scale_max, -160, 150);
                    widgets::SteppedSliderFloat("FFT Min", &fft_plot->scale_min, -160, 150);

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
                float right_width = win_size.x;
                float wf_size = win_size.y;
                bool show_waterfall = true;
                float waterfall_ratio = 0.3;
                float left_width = ImGui::GetCursorPosX() - 9;

                ImGui::BeginChild("RecorderFFT", {right_width, wf_size}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                {
                    float fft_height = wf_size * (show_waterfall ? waterfall_ratio : 1.0);
                    float wf_height = wf_size * (1 - waterfall_ratio) + 15 * ui_scale;
                    float wfft_widht = right_width - 9 * ui_scale;
                    bool t = true;
#ifdef __ANDROID__
                    int offset = 8;
#else
                    int offset = 30;
#endif
                    ImGui::SetNextWindowSizeConstraints(ImVec2((right_width + offset * ui_scale), 50), ImVec2((right_width + offset * ui_scale), wf_size));
                    ImGui::SetNextWindowSize(ImVec2((right_width + offset * ui_scale), show_waterfall ? waterfall_ratio * wf_size : wf_size));
                    ImGui::SetNextWindowPos(ImVec2(left_width, 25 * ui_scale));
                    if (ImGui::Begin("#fft", &t,
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoScrollbar |
                                         ImGuiWindowFlags_NoScrollWithMouse))
                    {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 9 * ui_scale);
                        fft_plot->draw({float(wfft_widht), fft_height});
                        if (show_waterfall && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                            waterfall_ratio = ImGui::GetWindowHeight() / wf_size;
                    }
                    ImGui::EndChild();
                    if (show_waterfall)
                    {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15 * ui_scale);
                        ImGui::SetCursorPosX(9 * ui_scale);
                        waterfall_plot->draw({wfft_widht, wf_height}, true);
                    }
                }
                ImGui::EndChild();
            }

            std::string getName() { return "RemoteHandleTest"; }

            std::string getID() { return "remote_test_handler"; }
        };
    } // namespace handlers
} // namespace satdump