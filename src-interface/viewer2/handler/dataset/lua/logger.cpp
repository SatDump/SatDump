#include "lua_bind.h"
#include "logger.h"

namespace satdump
{
    namespace lua
    {
        void bind_logger(sol::state &lua)
        {
            lua["ltrace"] = [](std::string log)
            { logger->trace("[Lua] " + log); };
            lua["ldebug"] = [](std::string log)
            { logger->debug("[Lua] " + log); };
            lua["linfo"] = [](std::string log)
            { logger->info("[Lua] " + log); };
            lua["lwarn"] = [](std::string log)
            { logger->warn("[Lua] " + log); };
            lua["lerror"] = [](std::string log)
            { logger->error("[Lua] " + log); };
            lua["lcritical"] = [](std::string log)
            { logger->critical("[Lua] " + log); };
        }
    }
}