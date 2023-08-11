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
    protected:
        void receive(slog::LogMsg log);
    public:
        StatusLoggerSink();
        ~StatusLoggerSink();
        int draw();
        bool is_shown();
    };
}