#pragma once

#include "base/remote_handler_backend.h"
#include "core/exception.h"
#include "dsp/device/dev.h"
#include "dsp/fft/fft_pan.h"
#include "dsp/path/splitter.h"
#include "nlohmann/json.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

namespace satdump
{
    namespace handlers
    {
        class RecBackend : public RemoteHandlerBackend
        {
        private:
            std::vector<ndsp::DeviceInfo> available_devices;
            ndsp::DeviceInfo current_device;

            bool is_started = false;

        private:
            std::shared_ptr<ndsp::DeviceBlock> dev_blk;

            std::shared_ptr<ndsp::SplitterBlock<complex_t>> splitter;

            std::shared_ptr<ndsp::FFTPanBlock> fftp;

            int fft_size = 65536;
            int fft_rate = 30;

        public:
            RecBackend()
            {
                available_devices = ndsp::getDeviceList(ndsp::DeviceBlock::MODE_SINGLE_RX);

                splitter = std::make_shared<ndsp::SplitterBlock<complex_t>>();

                fftp = std::make_shared<ndsp::FFTPanBlock>();
                fftp->set_input(splitter->add_output("main_fft"), 0);
                fftp->on_fft = [this](float *p, size_t s) { push_stream_data("fft", (uint8_t *)p, s * sizeof(float)); };
            }
            ~RecBackend() {}

            void start()
            {
                if (!dev_blk || is_started)
                    return;

                fftp->set_fft_settings(fft_size, dev_blk->getStreamSamplerate(0, false), fft_rate);

                splitter->link(dev_blk.get(), 0, 0, 100); //        fftp->inputs[0] = dev->outputs[0];

                dev_blk->start();
                splitter->start();
                fftp->start();

                is_started = true;
            }

            void stop()
            {
                if (!dev_blk || !is_started)
                    return;

                dev_blk->stop(true);
                splitter->stop();
                fftp->stop();

                is_started = false;
            }

            nlohmann::ordered_json _get_cfg_list()
            {
                nlohmann::ordered_json p;

                p["available_devices"]["type"] = "json";
                p["current_device"]["type"] = "json";

                if (dev_blk)
                {
                    p["dev/list"]["type"] = "json";
                    p["dev/cfg"]["type"] = "json";
                }

                p["started"]["type"] = "bool";

                p["fft/avg_num"]["type"] = "float";
                return p;
            }

            nlohmann::ordered_json _get_cfg(std::string key)
            {
                if (key == "available_devices")
                    return (nlohmann::json)available_devices;
                else if (key == "current_device")
                    return (nlohmann::json)current_device;
                else if (dev_blk && key == "dev/list")
                    return dev_blk->get_cfg_list();
                else if (dev_blk && key == "dev/cfg")
                    return dev_blk->get_cfg();
                else if (key == "started")
                    return is_started;
                else if (key == "fft/avg_num")
                    return fftp->avg_num;
                else if (key == "fft/size")
                    return fft_size;
                else if (key == "fft/rate")
                    return fft_rate;
                else
                    return nlohmann::json();
            }

            cfg_res_t _set_cfg(std::string key, nlohmann::ordered_json v)
            {
                if (!is_started && key == "current_device")
                {
                    bool found = false;
                    for (auto &d : available_devices)
                        found |= nlohmann::json(d) == nlohmann::json(v);

                    if (!found)
                        return RES_ERR;

                    current_device = v;
                    dev_blk = ndsp::getDeviceInstanceFromInfo(current_device, ndsp::DeviceBlock::MODE_SINGLE_RX);
                }
                else if (dev_blk && key == "dev/cfg")
                    dev_blk->set_cfg(v);
                else if (key == "started")
                {
                    bool val = v;
                    if (!is_started && val)
                        start();
                    else if (is_started && !val)
                        stop();
                }
                else if (key == "fft/avg_num")
                {
                    fftp->avg_num = v;
                }
                else if (key == "fft/size")
                {
                    fft_size = v;

                    if (fft_size < 8)
                        fft_size = 8;
                    if (fft_size > 65536 * 8)
                        fft_size = 65536 * 8;

                    if (is_started)
                        fftp->set_fft_settings(fft_size, dev_blk->getStreamSamplerate(0, false), fft_rate);
                }
                else if (key == "fft/rate")
                {
                    fft_rate = v;

                    if (fft_rate < 1)
                        fft_rate = 1;
                    if (fft_rate > 100e3)
                        fft_rate = 100e3;

                    if (is_started)
                        fftp->set_fft_settings(fft_size, dev_blk->getStreamSamplerate(0, false), fft_rate);
                }
                else
                    throw satdump_exception("Oops");

                return RES_OK;
            }
        };
    } // namespace handlers
} // namespace satdump