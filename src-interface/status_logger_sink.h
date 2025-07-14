#pragma once

#include "common/widgets/logger_sink.h"

namespace satdump
{
    class StatusLoggerSink : public widgets::LoggerSinkWidget
    {
    private:
        std::string str;
        std::string lvl;
        bool show_bar;
        bool show_log;

    private:
        int processing_tasks_n = 0;

    protected:
        void receive(slog::LogMsg log);

    public:
        StatusLoggerSink();
        ~StatusLoggerSink();
        int draw();
        bool is_shown();
    };
} // namespace satdump