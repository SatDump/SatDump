#pragma once

#include "libs/fmt/fmt.h"
#include <memory>
#include "dll_export.h"
#include <vector>
#include <fstream>

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

        std::string format_log(LogMsg m, bool color);

    public:
        virtual void receive(LogMsg log) = 0;
        void set_level(LogLevel lvl) { sink_lvl = lvl; }
    };

    class Logger
    {
    private:
        LogLevel logger_lvl = LOG_TRACE;
        std::vector<std::shared_ptr<LoggerSink>> sinks;

    public:
        void set_level(LogLevel lvl);
        void add_sink(std::shared_ptr<LoggerSink> sink);
        void del_sink(std::shared_ptr<LoggerSink> sink);

    public:
        // FMT formats
        template <typename... T>
        void trace(fmt::format_string<T...> fmt, T &&...args) { log(LOG_TRACE, fmt, args...); }
        template <typename... T>
        void debug(fmt::format_string<T...> fmt, T &&...args) { log(LOG_DEBUG, fmt, args...); }
        template <typename... T>
        void info(fmt::format_string<T...> fmt, T &&...args) { log(LOG_INFO, fmt, args...); }
        template <typename... T>
        void warn(fmt::format_string<T...> fmt, T &&...args) { log(LOG_WARN, fmt, args...); }
        template <typename... T>
        void error(fmt::format_string<T...> fmt, T &&...args) { log(LOG_ERROR, fmt, args...); }
        template <typename... T>
        void critical(fmt::format_string<T...> fmt, T &&...args) { log(LOG_CRIT, fmt, args...); }

        // Int formats
        void trace(int v) { log(LOG_TRACE, v); }
        void debug(int v) { log(LOG_DEBUG, v); }
        void info(int v) { log(LOG_INFO, v); }
        void warn(int v) { log(LOG_WARN, v); }
        void error(int v) { log(LOG_ERROR, v); }
        void critical(int v) { log(LOG_CRIT, v); }

        // String formats
        void trace(std::string v) { log(LOG_TRACE, v); }
        void debug(std::string v) { log(LOG_DEBUG, v); }
        void info(std::string v) { log(LOG_INFO, v); }
        void warn(std::string v) { log(LOG_WARN, v); }
        void error(std::string v) { log(LOG_ERROR, v); }
        void critical(std::string v) { log(LOG_CRIT, v); }

        template <typename... T>
        void log(LogLevel lvl, fmt::format_string<T...> fmt, T &&...args) { log(lvl, fmt::format(fmt, args...)); }

        void log(LogLevel lvl, int v) { log(lvl, std::to_string(v)); }

        void log(LogLevel lvl, std::string v);
    };

    class StdOutSink : public LoggerSink
    {
    public:
        void receive(LogMsg log);
    };

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

// Change output level, eg Debug, Info, etc
void setConsoleLevel(slog::LogLevel level);