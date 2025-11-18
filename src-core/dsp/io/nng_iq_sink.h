#pragma once

#include "dsp/block.h"
#include <nng/nng.h>

namespace satdump
{
    namespace ndsp
    {
        class NNGIQSinkBlock : public Block
        {

        private:
            std::string address = "0.0.0.0";
            int port = 8888;

            nng_socket sock;
            nng_listener listener;

            bool work();

        public:
            void start();
            void stop(bool stop_now = false);

        public:
            NNGIQSinkBlock();
            ~NNGIQSinkBlock();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "address", "string");
                p["address"]["disable"] = is_work_running();
                add_param_simple(p, "port", "int");
                p["port"]["disable"] = is_work_running();
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "address")
                    return address;
                else if (key == "port")
                    return port;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "address")
                    address = v;
                else if (key == "port")
                    port = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump