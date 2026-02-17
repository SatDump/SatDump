#pragma once

#include "base/remote_handler.h"
#include "base/remote_handler_backend.h"
#include "common/widgets/fft_plot.h"
#include "dsp/device/dev.h"
#include "dsp/device/options_displayer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "utils/task_queue.h"
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

            bool is_started = false;

        protected:
            bool mustUpdate = true;
            void handle_stream_data(std::string id, uint8_t *data, size_t size)
            {
                if (id == "upd")
                {
                    mustUpdate = true;
                }
                else if (id == "fft")
                {
                    memcpy(fft_vec.data(), data, size);
                }
            }

        public:
            RecFrontendHandler(std::shared_ptr<RemoteHandlerBackend> bkd) : RemoteHandlerHandler(bkd)
            {
                fft_vec.resize(65536);

                fft_plot = std::make_shared<widgets::FFTPlot>(fft_vec.data(), 65536, -150, 150, 10);
                fft_plot->frequency = 431.8e6;
                fft_plot->enable_freq_scale = true;
            }
            ~RecFrontendHandler() {}

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

                            is_started = bkd->get_cfg("started");
                        });

                    mustUpdate = false;
                }

                std::scoped_lock l(fm);

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
            }

            void drawContents(ImVec2 win_size) { fft_plot->draw(win_size); }

            std::string getName() { return "RemoteHandleTest"; }

            std::string getID() { return "remote_test_handler"; }
        };
    } // namespace handlers
} // namespace satdump