#include "remote_handler.h"
#include <cstdint>

namespace satdump
{
    namespace handlers
    {
        RemoteHandlerHandler::RemoteHandlerHandler(std::shared_ptr<RemoteHandlerBackend> bkd) : bkd(bkd)
        {
            bkd->set_stream_rx_handler([this](std::string v, uint8_t *dat, size_t sz) { handle_stream_data(v, dat, sz); });
        }

        RemoteHandlerHandler::~RemoteHandlerHandler() {}
    } // namespace handlers
} // namespace satdump