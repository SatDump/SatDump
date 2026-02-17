#pragma once

#include "../base/remote_handler_backend.h"
#include "core/exception.h"
#include "logger.h"
#include "nlohmann/json.hpp"
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

namespace satdump
{
    namespace handlers
    {
        class TestHttpBackend : public RemoteHandlerBackend
        {
        private:
            std::shared_ptr<RemoteHandlerBackend> bkd2;

            nng_http_server *http_server;
            nng_http_handler *handler_api;
            nng_http_handler *handler_apiG;
            nng_http_handler *handler_apiP;
            nng_socket socket;

        private:
            // HTTP Handler for stats
            static void http_handle(nng_aio *aio)
            {
                nlohmann::json output;

                nng_http_handler *handler = (nng_http_handler *)nng_aio_get_input(aio, 1);
                TestHttpBackend *tthis = (TestHttpBackend *)nng_http_handler_get_data(handler);

                output = tthis->bkd2->get_cfg_list();

                std::string jsonstr = output.dump();

                nng_http_res *res;
                nng_http_res_alloc(&res);
                nng_http_res_copy_data(res, jsonstr.c_str(), jsonstr.size());
                nng_http_res_set_header(res, "Content-Type", "application/json; charset=utf-8");
                nng_aio_set_output(aio, 0, res);
                nng_aio_finish(aio, 0);
            }

            // HTTP Handler for commands
            static void http_handleG(nng_aio *aio)
            {
                nng_http_req *msg = (nng_http_req *)nng_aio_get_input(aio, 0);
                nng_http_handler *handler = (nng_http_handler *)nng_aio_get_input(aio, 1);
                TestHttpBackend *tthis = (TestHttpBackend *)nng_http_handler_get_data(handler);

                void *ptr;
                size_t ptrl;
                nng_http_req_get_data(msg, &ptr, &ptrl);
                //  logger->info("Got : %s", (char *)ptr);

                nlohmann::json output;

                output = tthis->bkd2->get_cfg(std::string((char *)ptr, ptrl));

                std::string jsonstr = output.dump();

                nng_http_res *res;
                nng_http_res_alloc(&res);
                nng_http_res_copy_data(res, jsonstr.c_str(), jsonstr.size());
                nng_http_res_set_header(res, "Content-Type", "application/json; charset=utf-8");
                nng_aio_set_output(aio, 0, res);
                nng_aio_finish(aio, 0);
            }

            // HTTP Handler for commands
            static void http_handleP(nng_aio *aio)
            {
                nng_http_req *msg = (nng_http_req *)nng_aio_get_input(aio, 0);
                nng_http_handler *handler = (nng_http_handler *)nng_aio_get_input(aio, 1);
                TestHttpBackend *tthis = (TestHttpBackend *)nng_http_handler_get_data(handler);

                void *ptr;
                size_t ptrl;
                nng_http_req_get_data(msg, &ptr, &ptrl);
                // logger->info("Got : %s", (char *)ptr);

                nlohmann::json input;
                nlohmann::json output;

                try
                {
                    input = nlohmann::json::parse(std::string((char *)ptr, ptrl));
                    output["res"] = tthis->bkd2->set_cfg(input);
                }
                catch (std::exception &e)
                {
                    output["res"] = 2;
                    output["err"] = e.what();
                    output["input"] = std::string((char *)ptr, ptrl);
                }

                std::string jsonstr = output.dump();

                nng_http_res *res;
                nng_http_res_alloc(&res);
                nng_http_res_copy_data(res, jsonstr.c_str(), jsonstr.size());
                nng_http_res_set_header(res, "Content-Type", "application/json; charset=utf-8");
                nng_aio_set_output(aio, 0, res);
                nng_aio_finish(aio, 0);
            }

        public:
            TestHttpBackend(std::shared_ptr<RemoteHandlerBackend> bkd2) : bkd2(bkd2)
            {
                bkd2->set_stream_rx_handler(
                    [this](std::string v, uint8_t *dat, size_t sz)
                    {
                        // logger->info("Stream " + v);
                        push_stream_data(v, dat, sz);

                        uint32_t fftsz = sz;

                        std::vector<uint8_t> send_buf;
                        send_buf.push_back(v.size());
                        send_buf.insert(send_buf.end(), v.begin(), v.begin() + v.size());
                        send_buf.push_back((fftsz >> 24) & 0xFF);
                        send_buf.push_back((fftsz >> 16) & 0xFF);
                        send_buf.push_back((fftsz >> 8) & 0xFF);
                        send_buf.push_back((fftsz >> 0) & 0xFF);
                        send_buf.insert(send_buf.end(), dat, dat + sz);
                        nng_send(socket, send_buf.data(), send_buf.size(), NNG_FLAG_NONBLOCK);
                    });

                std::string http_server_url = "http://0.0.0.0:8080";

                nng_url *url;
                {
                    nng_url_parse(&url, http_server_url.c_str());
                    nng_http_server_hold(&http_server, url);
                }

                {
                    nng_http_handler_alloc(&handler_api, "/list", http_handle);
                    nng_http_handler_set_data(handler_api, this, 0);
                    nng_http_handler_set_method(handler_api, "GET");
                    nng_http_server_add_handler(http_server, handler_api);
                }

                {
                    nng_http_handler_alloc(&handler_apiP, "/get", http_handleG);
                    nng_http_handler_set_data(handler_apiP, this, 0);
                    nng_http_handler_set_method(handler_apiP, "POST");
                    nng_http_server_add_handler(http_server, handler_apiP);
                }

                {
                    nng_http_handler_alloc(&handler_apiP, "/set", http_handleP);
                    nng_http_handler_set_data(handler_apiP, this, 0);
                    nng_http_handler_set_method(handler_apiP, "POST");
                    nng_http_server_add_handler(http_server, handler_apiP);
                }

                {

                    int rv = 0;
                    if (rv = nng_bus0_open(&socket); rv != 0)
                    {
                        printf("pair open error\n");
                    }

                    if (rv = nng_listen(socket, "ws://0.0.0.0:8080/ws", nullptr, 0); rv != 0)
                    {
                        printf("server listen error\n");
                    }
                }

                nng_url_free(url);

                nng_http_server_start(http_server);
            }

            ~TestHttpBackend() {}

            nlohmann::ordered_json _get_cfg_list() { return bkd2->get_cfg_list(); }
            nlohmann::ordered_json _get_cfg(std::string key) { return bkd2->get_cfg(key); }
            cfg_res_t _set_cfg(std::string key, nlohmann::ordered_json v) { return bkd2->set_cfg(key, v); }
        };
    } // namespace handlers
} // namespace satdump