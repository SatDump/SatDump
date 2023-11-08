#pragma once

#include <mutex>
#include "logger.h"

namespace satdump
{
    class NotifyLoggerSink : public slog::LoggerSink
    {
    public:
        NotifyLoggerSink();
        ~NotifyLoggerSink();
        void receive(slog::LogMsg log);

        std::mutex notify_mutex;
    };
}