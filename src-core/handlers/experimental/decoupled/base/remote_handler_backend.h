#pragma once

#include "nlohmann/json.hpp"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <string>

namespace satdump
{
    namespace handlers
    {
        class RemoteHandlerBackend
        {
        protected:
            std::function<void(std::string, uint8_t *, size_t)> stream_rx;
            std::mutex stream_rx_mtx;

            void push_stream_data(std::string id, uint8_t *data, size_t size)
            {
                std::scoped_lock l(stream_rx_mtx);
                stream_rx(id, data, size);
            }

        public:
            void set_stream_rx_handler(std::function<void(std::string, uint8_t *, size_t)> f)
            {
                std::scoped_lock l(stream_rx_mtx);
                stream_rx = f;
            }

        public:
            enum cfg_res_t
            {
                RES_OK = 0,
                RES_LISTUPD = 1,
                RES_ERR = 2,
            };

        private:
            std::mutex cfg_mtx;

            virtual nlohmann::ordered_json _get_cfg_list() = 0;
            virtual nlohmann::ordered_json _get_cfg(std::string key) = 0;
            virtual cfg_res_t _set_cfg(std::string key, nlohmann::ordered_json v) = 0;

        public:
            nlohmann::ordered_json get_cfg_list()
            {
                std::scoped_lock l(cfg_mtx);
                return _get_cfg_list();
            }

            nlohmann::ordered_json get_cfg(std::string key)
            {
                std::scoped_lock l(cfg_mtx);
                return _get_cfg(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::ordered_json v)
            {
                std::scoped_lock l(cfg_mtx);
                cfg_res_t r = RES_ERR;
                try
                {
                    r = _set_cfg(key, v);
                }
                catch (std::exception &e)
                {
                }

                push_stream_data("upd", NULL, 0);
                return r;
            }

            nlohmann::ordered_json get_cfg()
            {
                nlohmann::ordered_json p;
                auto v = get_cfg_list();
                for (auto &v2 : v.items())
                    p[v2.key()] = get_cfg(v2.key());
                return p;
            }

            cfg_res_t set_cfg(nlohmann::ordered_json v)
            {
                cfg_res_t r = RES_OK;
                for (auto &i : v.items())
                {
                    cfg_res_t r2 = set_cfg(i.key(), i.value());
                    if (r2 > r)
                        r = r2;
                }
                return r;
            }

        public:
            RemoteHandlerBackend() {}
            ~RemoteHandlerBackend() {}

        protected:
            //

        public:
            //
        };
    } // namespace handlers
} // namespace satdump