#pragma once

#include "handlers/handler.h"
#include "remote_handler_backend.h"
#include <memory>

namespace satdump
{
    namespace handlers
    {
        class RemoteHandlerHandler : public Handler
        {
        protected:
            std::shared_ptr<RemoteHandlerBackend> bkd;

            virtual void handle_stream_data(std::string, uint8_t *, size_t) = 0;

        public:
            RemoteHandlerHandler(std::shared_ptr<RemoteHandlerBackend> bkd);
            ~RemoteHandlerHandler();
        };
    } // namespace handlers
} // namespace satdump