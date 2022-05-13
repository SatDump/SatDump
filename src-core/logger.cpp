#define SATDUMP_DLL_EXPORT 1
#include "logger.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#ifdef __ANDROID__
#include <spdlog/sinks/android_sink.h>
#endif

// Logger and sinks. We got a console sink and file sink
#ifdef __ANDROID__
std::shared_ptr<spdlog::sinks::android_sink_mt> console_sink;
#else
std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink;
#endif
std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file_sink;
SATDUMP_DLL std::shared_ptr<spdlog::logger> logger;

void initLogger()
{
    try
    {
// Initialize everything
#ifdef __ANDROID__
        console_sink = std::make_shared<spdlog::sinks::android_sink_mt>();
#else
        console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#endif

#ifndef __ANDROID__
        file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("satdump.logs", true);
#endif

#ifndef __ANDROID__
        logger = std::shared_ptr<spdlog::logger>(new spdlog::logger("SatDump", {console_sink, file_sink}));
#else
        logger = std::shared_ptr<spdlog::logger>(new spdlog::logger("SatDump", {console_sink}));
#endif

        // Use a custom, nicer log pattern. No color in the file
        console_sink->set_pattern("[%D - %T] %^(%L) %v%$");
#ifndef __ANDROID__
        file_sink->set_pattern("[%D - %T] (%L) %v");
#endif

// Default log level
#ifndef __ANDROID__
        file_sink->set_level(spdlog::level::trace);
#endif
        console_sink->set_level(spdlog::level::trace);
        logger->set_level(spdlog::level::trace);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

void setConsoleLevel(spdlog::level::level_enum level)
{
    // Just change our log level
    console_sink->set_level(level);
}