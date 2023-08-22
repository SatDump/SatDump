#pragma once

#include <deque>
#include "logger.h"

namespace widgets
{
    class LoggerSinkWidget : public slog::LoggerSink
    {
    private:
        struct LogLine
        {
            slog::LogLevel lvl;
            std::string str;
        };

        std::deque<LogLine> all_lines;
        std::mutex mtx;
        bool new_item = false;

    protected:
        void receive(slog::LogMsg log);

    public:
        size_t max_lines = 1e3;
        void draw();
    };
}