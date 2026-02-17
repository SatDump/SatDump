#pragma once

#include "../base/remote_handler_backend.h"
#include "core/exception.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "utils/http.h"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <nng/nng.h>
#include <nng/protocol/bus0/bus.h>
#include <nng/protocol/pair0/pair.h>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/util/platform.h>
#include <thread>

namespace satdump
{
    namespace handlers
    {
        class TestHttpClientBackend : public RemoteHandlerBackend
        {
        private:
            nng_socket socket;
            nng_dialer dialer;

            bool thread_should_run = true;
            std::thread rx_th;

        public:
            TestHttpClientBackend()
            {
                {

                    int rv = 0;
                    if (rv = nng_bus0_open(&socket); rv != 0)
                    {
                        printf("pair open error\n");
                    }

                    if (rv = nng_dial(socket, "ws://0.0.0.0:8080/ws", &dialer, 0); rv != 0)
                    {
                        printf("server listen error\n");
                    }
                }

                rx_th = std::thread(&TestHttpClientBackend::rxThread, this);
            }

            ~TestHttpClientBackend()
            {
                thread_should_run = false;
                if (rx_th.joinable())
                    rx_th.join();
            }

            void rxThread()
            {
                while (thread_should_run)
                {
                    void *ptr;
                    size_t sz = 0;
                    nng_recv(socket, &ptr, &sz, NNG_FLAG_ALLOC | NNG_FLAG_NONBLOCK);

                    if (ptr != NULL && sz > 0)
                    {
                        uint8_t *dat = (uint8_t *)ptr;
                        std::string id((char *)dat + 1, dat[0]);
                        uint8_t *payload = dat + 1 + (int)dat[0];
                        size_t psize = payload[0] << 24 | payload[1] << 16 | payload[2] << 8 | payload[3];
                        payload += 4;
                        // logger->info("Payload %s %d", id.c_str(), psize);

                        push_stream_data(id, payload, psize);

                        nng_free(ptr, sz);
                    }
                }
            }

            nlohmann::ordered_json _get_cfg_list()
            {
                std::string res;
                perform_http_request("http://0.0.0.0:8080/list", res);
                return nlohmann::ordered_json::parse(res);
            }

            nlohmann::ordered_json _get_cfg(std::string key)
            {
                std::string res;
                perform_http_request_post("http://0.0.0.0:8080/get", res, key);
                return nlohmann::ordered_json::parse(res);
            }

            cfg_res_t _set_cfg(std::string key, nlohmann::ordered_json v)
            {
                std::string res;
                nlohmann::json vs;
                vs[key] = v;
                perform_http_request_post("http://0.0.0.0:8080/set", res, vs.dump());
                return nlohmann::ordered_json::parse(res)["res"];
            }
        };
    } // namespace handlers
} // namespace satdump