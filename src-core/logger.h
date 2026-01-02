#pragma once

#include <memory>
#include "dll_export.h"
#include <vector>
#include <fstream>
#include <mutex>
#include <stdarg.h>
#include <string>

namespace slog
{
    // Logger log level
    enum LogLevel
    {
        LOG_TRACE,
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR,
        LOG_CRIT,
        LOG_OFF,
    };

    // Struct holding a formatted message
    // Including special color fomatting
    // chars
    struct LogMsg
    {
        std::string str;
        LogLevel lvl;
    };

    // Logger sink
    class LoggerSink
    {
    protected:
        LogLevel sink_lvl = LOG_TRACE;

        std::string format_log(LogMsg m, bool color, int *cpos = nullptr);

    public:
        virtual void receive(LogMsg log) = 0;
        void set_level(LogLevel lvl) { sink_lvl = lvl; }
    };

    class Logger
    {
    private:
        LogLevel logger_lvl = LOG_TRACE;
        std::mutex sink_mtx;
        std::vector<std::shared_ptr<LoggerSink>> sinks;

    public:
        void set_level(LogLevel lvl);
        LogLevel get_level();
        void add_sink(std::shared_ptr<LoggerSink> sink);
        void del_sink(std::shared_ptr<LoggerSink> sink);

    public:
        // FMT formats
        void trace(std::string fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            logf(LOG_TRACE, fmt, args);
            va_end(args);
        }
        void debug(std::string fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            logf(LOG_DEBUG, fmt, args);
            va_end(args);
        }
        void info(std::string fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            logf(LOG_INFO, fmt, args);
            va_end(args);
        }
        void warn(std::string fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            logf(LOG_WARN, fmt, args);
            va_end(args);
        }
        void error(std::string fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            logf(LOG_ERROR, fmt, args);
            va_end(args);
        }
        void critical(std::string fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            logf(LOG_CRIT, fmt, args);
            va_end(args);
        }

        // Int formats
        void trace(int v) { log(LOG_TRACE, v); }
        void debug(int v) { log(LOG_DEBUG, v); }
        void info(int v) { log(LOG_INFO, v); }
        void warn(int v) { log(LOG_WARN, v); }
        void error(int v) { log(LOG_ERROR, v); }
        void critical(int v) { log(LOG_CRIT, v); }

        void logf(LogLevel lvl, std::string fmt, va_list args);
        void log(LogLevel lvl, int v) { log(lvl, std::to_string(v)); }
        void log(LogLevel lvl, std::string v);
    };

    class StdOutSink : public LoggerSink
    {
    public:
        void receive(LogMsg log);
    };

#if defined(_WIN32)
    class WinOutSink : public LoggerSink
    {
    public:
        void receive(LogMsg log);
    };
#endif

#ifdef __ANDROID__
    class AndroidSink : public LoggerSink
    {
    public:
        void receive(LogMsg log);
    };
#endif

    class FileSink : public LoggerSink
    {
    private:
        std::ofstream outf;

    public:
        FileSink(std::string path);
        ~FileSink();
        void receive(LogMsg log);
    };
}

// Main logger instance
SATDUMP_DLL extern std::shared_ptr<slog::Logger> logger;

// Initialize the logger
void initLogger();

// Init logger file sink
void initFileSink();

// Tell the logger that initialization is done so it stops buffering messages
void completeLoggerInit();

// Change output level, eg Debug, Info, etc
void setConsoleLevel(slog::LogLevel level);