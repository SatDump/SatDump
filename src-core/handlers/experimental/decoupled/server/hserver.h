#pragma once

#include "handlers/experimental/decoupled/base/remote_handler.h"
#include <exception>
#include <memory>

namespace satdump
{
    namespace handlers
    {
        namespace hbackend_server_pkts
        {
            struct HandlerList
            {
            };
        } // namespace hbackend_server_pkts

        class HBackendServer
        {
        private:
            struct BackendHandlerInfo
            {
                std::string id;
                std::shared_ptr<RemoteHandlerHandler> h;
            };

        public:
            HBackendServer() {}
            ~HBackendServer() {}

            nlohmann::json command(nlohmann::json c)
            {
                try
                {
                    std::string type = c["type"];
                }
                catch (std::exception &e)
                {
                }
            }
        };
    } // namespace handlers
} // namespace satdump