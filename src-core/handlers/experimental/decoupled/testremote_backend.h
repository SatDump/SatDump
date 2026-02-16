#pragma once

#include "base/remote_handler_backend.h"
#include "core/exception.h"
#include "nlohmann/json.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

namespace satdump
{
    namespace handlers
    {
        class TestRemoteHandlerBackend : public RemoteHandlerBackend
        {
        private:
            double val = 0;

        public:
            TestRemoteHandlerBackend() {}
            ~TestRemoteHandlerBackend() {}

            nlohmann::ordered_json _get_cfg_list()
            {
                nlohmann::ordered_json p;
                p["val"]["type"] = "float";
                return p;
            }

            nlohmann::ordered_json _get_cfg(std::string key)
            {
                if (key == "val")
                    return val;
                else
                    throw satdump_exception("Oops");
            }

            cfg_res_t _set_cfg(std::string key, nlohmann::ordered_json v)
            {
                if (key == "val")
                {
                    if (v < 0)
                        v = -100;

                    val = v;
                }
                else
                    throw satdump_exception("Oops");

                return RES_OK;
            }
        };
    } // namespace handlers
} // namespace satdump