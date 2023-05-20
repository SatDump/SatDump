#pragma once

#include "logger.h"

namespace satdump
{
    class NotifyLoggerSink : public slog::LoggerSink
    {
    public:
        NotifyLoggerSink();
        ~NotifyLoggerSink();
        void receive(slog::LogMsg log);
    };
}