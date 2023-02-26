#define SATDUMP_DLL_EXPORT 1
#include "logger.h"
#include <iostream>
#include <ctime>
#include <algorithm>
#ifdef __ANDROID__
#include <android/log.h>
static char ag_LogTag[] = "SatDump";
#endif

// Logger and sinks. We got a console sink and file sink
#ifdef __ANDROID__
std::shared_ptr<slog::AndroidSink> console_sink;
#else
std::shared_ptr<slog::StdOutSink> console_sink;
#endif
std::shared_ptr<slog::FileSink> file_sink;
SATDUMP_DLL std::shared_ptr<slog::Logger> logger;

namespace slog
{
    const std::string log_schar[] = {"T", "D", "I", "W", "E", "C"};
    const std::string colors[] = {"\033[37m", "\033[36m", "\033[32m", "\033[33m\033[1m", "\033[31m\033[1m", "\033[1m\033[41m"};

    std::string LoggerSink::format_log(LogMsg m, bool color)
    {
        time_t ct = time(0);
        std::tm *tmr = gmtime(&ct);

        std::string timestamp =
            (tmr->tm_hour < 10 ? "0" : "") + std::to_string(tmr->tm_hour) + ":" + // Hour
            (tmr->tm_min < 10 ? "0" : "") + std::to_string(tmr->tm_min) + ":" +   // Min
            (tmr->tm_sec < 10 ? "0" : "") + std::to_string(tmr->tm_sec) + " - " + // Sec
            (tmr->tm_mday < 10 ? "0" : "") + std::to_string(tmr->tm_mday) + "/" + // Day
            (tmr->tm_mon < 10 ? "0" : "") + std::to_string(tmr->tm_mon) + "/" +   // Mon
            (tmr->tm_year < 10 ? "0" : "") + std::to_string(tmr->tm_year + 1900); // Year

        if (color)
            return fmt::format("[{:s}] {:s}({:s}) {:s}\033[m\n",
                               timestamp.c_str(),
                               colors[m.lvl].c_str(), log_schar[m.lvl].c_str(), m.str.c_str());
        else
            return fmt::format("[{:s}] ({:s}) {:s}\n",
                               timestamp.c_str(),
                               log_schar[m.lvl].c_str(), m.str.c_str());
    }

    void StdOutSink::receive(LogMsg log)
    {
        if (log.lvl >= sink_lvl)
        {
            std::string s = format_log(log, true);
            fwrite(s.c_str(), sizeof(char), s.size(), stderr);
        }
    }

    FileSink::FileSink(std::string path)
    {
        outf = std::ofstream(path);
    }

    FileSink::~FileSink()
    {
        outf.close();
    }

    void FileSink::receive(LogMsg log)
    {
        if (log.lvl >= sink_lvl)
        {
            std::string s = format_log(log, false);
            outf.write(s.c_str(), s.size());
            outf.flush(); // Too annoying when there's a segault not letting it flush
        }
    }

    void Logger::log(LogLevel lvl, std::string v)
    {
        LogMsg m;
        m.str = v;
        m.lvl = lvl;
        if (m.lvl >= logger_lvl)
            for (auto &l : sinks)
                l->receive(m);
    }

#ifdef __ANDROID__
    const android_LogPriority log_lvls_a[] = {
        ANDROID_LOG_VERBOSE,
        ANDROID_LOG_DEBUG,
        ANDROID_LOG_INFO,
        ANDROID_LOG_WARN,
        ANDROID_LOG_ERROR,
        ANDROID_LOG_FATAL,
    };
    void AndroidSink::receive(LogMsg log)
    {
        if (log.lvl >= sink_lvl)
        {
            std::string s = format_log(log, false);
            __android_log_print(log_lvls_a[log.lvl], ag_LogTag, "%s", log.str.c_str());
        }
    }
#endif

    void Logger::set_level(LogLevel lvl)
    {
        logger_lvl = lvl;
    }

    void Logger::add_sink(std::shared_ptr<LoggerSink> sink)
    {
        sinks.push_back(sink);
    }

    void Logger::del_sink(std::shared_ptr<LoggerSink> sink)
    {
        auto it = std::find_if(sinks.rbegin(), sinks.rend(), [&sink](std::shared_ptr<LoggerSink> &c)
                               { return sink.get() == c.get(); })
                      .base();
        if (it != sinks.end())
            sinks.erase(it);
    }
}

void initLogger()
{
    try
    {
// Initialize everything
#ifdef __ANDROID__
        console_sink = std::make_shared<slog::AndroidSink>();
#else
        console_sink = std::make_shared<slog::StdOutSink>();
#endif

        // logger = std::shared_ptr<spdlog::logger>(new spdlog::logger("SatDump", {console_sink}));
        logger = std::make_shared<slog::Logger>();
        logger->add_sink(console_sink);

        // Use a custom, nicer log pattern. No color in the file
        // console_sink->set_pattern("[%D - %T] %^(%L) %v%$");

        // Default log level
        console_sink->set_level(slog::LOG_TRACE);
        logger->set_level(slog::LOG_TRACE);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

void initFileSink()
{
    try
    {
        file_sink = std::make_shared<slog::FileSink>("satdump.logs");
        logger->add_sink(file_sink);
        // file_sink->set_pattern("[%D - %T] (%L) %v");
        file_sink->set_level(slog::LOG_TRACE);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

void setConsoleLevel(slog::LogLevel level)
{
    // Just change our log level
    console_sink->set_level(level);
}