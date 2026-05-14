#pragma once

#include "common/net/udp.h"
#include "common/utils.h"
#include "dsp/block.h"
#include "logger.h"
#include <fstream>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class UDPSourceBlock : public Block
        {
        private:
            int max_pkt_size = 65536;
            int port = 8888;

            std::shared_ptr<net::UDPServer> udp_rx;

            bool work();

        public:
            UDPSourceBlock();
            ~UDPSourceBlock();

            void init()
            {
                udp_rx = std::make_shared<net::UDPServer>(port);
                udp_rx->enableTimeout();
            }

            void stop(bool stop_now = false, bool force = false)
            {
                Block::stop(stop_now, force);
                udp_rx.reset();
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "max_pkt_size", "int");
                p["max_pkt_size"]["disable"] = is_work_running();
                add_param_simple(p, "port", "int");
                p["port"]["disable"] = is_work_running();
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "max_pkt_size")
                    return max_pkt_size;
                else if (key == "port")
                    return port;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "max_pkt_size")
                    max_pkt_size = v;
                else if (key == "port")
                    port = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump