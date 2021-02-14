#include "logger.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

// Logger and sinks. We got a console sink and file sink
std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink;
std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file_sink;
std::shared_ptr<spdlog::logger> logger;

void initLogger()
{
    try
    {
        // Initialize everything
        console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("satdump.logs", true);
        logger = std::shared_ptr<spdlog::logger>(new spdlog::logger("SatDump", {console_sink, file_sink}));

        // Use a custom, nicer log pattern. No color in the file
        console_sink->set_pattern("[%D - %T] %^(%L) %v%$");
        file_sink->set_pattern("[%D - %T] (%L) %v");

        // Default log level
        file_sink->set_level(spdlog::level::trace);
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