#pragma once

#include "base/remote_handler_backend.h"
#include "core/exception.h"
#include "dsp/device/dev.h"
#include "dsp/fft/fft_pan.h"
#include "dsp/path/splitter.h"
#include "nlohmann/json.hpp"
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

        public:
            RecBackend()
            {
                available_devices = ndsp::getDeviceList(ndsp::DeviceBlock::MODE_SINGLE_RX);

                splitter = std::make_shared<ndsp::SplitterBlock<complex_t>>();

                fftp = std::make_shared<ndsp::FFTPanBlock>();
                fftp->set_input(splitter->add_output("main_fft"), 0);
                fftp->on_fft = [this](float *p) { push_stream_data("fft", (uint8_t *)p, 65536 * sizeof(float)); };
            }
            ~RecBackend() {}

            void start()
            {
                if (!dev_blk || is_started)
                    return;

                fftp->set_fft_settings(65536, dev_blk->getStreamSamplerate(0, false), 30);
                fftp->avg_num = 1;

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
                else
                    throw satdump_exception("Oops");
            }

            cfg_res_t _set_cfg(std::string key, nlohmann::ordered_json v)
            {
                if (key == "current_device")
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
                else
                    throw satdump_exception("Oops");

                return RES_OK;
            }
        };
    } // namespace handlers
} // namespace satdump