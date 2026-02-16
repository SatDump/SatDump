#pragma once

#include "base/remote_handler_backend.h"
#include "core/exception.h"
#include "dsp/device/dev.h"
#include "nlohmann/json.hpp"
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

        private:
            std::shared_ptr<ndsp::DeviceBlock> dev_blk;

        public:
            RecBackend() { available_devices = ndsp::getDeviceList(ndsp::DeviceBlock::MODE_SINGLE_RX); }
            ~RecBackend() {}

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
                else
                    throw satdump_exception("Oops");

                return RES_OK;
            }
        };
    } // namespace handlers
} // namespace satdump