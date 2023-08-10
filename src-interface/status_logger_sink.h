#pragma once

#include "logger.h"

namespace satdump
{
    class StatusLoggerSink : public slog::LoggerSink
    {
    private:
        std::string str;
        std::string lvl;
    public:
        StatusLoggerSink();
        ~StatusLoggerSink();
        void receive(slog::LogMsg log);
        int draw();
    };
}