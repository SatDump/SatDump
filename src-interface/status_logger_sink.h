#pragma once

#include "logger.h"

namespace satdump
{
    class StatusLoggerSink : public slog::LoggerSink
    {
    private:
        std::string str;
        std::string lvl;
        bool show;
    public:
        StatusLoggerSink();
        ~StatusLoggerSink();
        void receive(slog::LogMsg log);
        int draw();
        bool is_shown();
    };
}