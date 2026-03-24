#pragma once

#include "handlers/experimental/decoupled/base/remote_handler.h"
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
        };
    } // namespace handlers
} // namespace satdump