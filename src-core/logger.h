#pragma once

#include <spdlog/spdlog.h>
#include <memory>
#include "dll_export.h"

// Main logger instance
SATDUMP_DLL extern std::shared_ptr<spdlog::logger> logger;

// Initialize the logger
void initLogger();

// Change output level, eg Debug, Info, etc
void setConsoleLevel(spdlog::level::level_enum level);