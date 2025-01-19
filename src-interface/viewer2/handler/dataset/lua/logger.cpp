#include "lua_bind.h"
#include "logger.h"

namespace satdump
{
    namespace lua
    {
        void bind_logger(sol::state &lua)
        {
            lua["ltrace"] = [](std::string log)
            { logger->trace(log); };
            lua["ldebug"] = [](std::string log)
            { logger->debug(log); };
            lua["linfo"] = [](std::string log)
            { logger->info(log); };
            lua["lwarn"] = [](std::string log)
            { logger->warn(log); };
            lua["lerror"] = [](std::string log)
            { logger->error(log); };
            lua["lcritical"] = [](std::string log)
            { logger->critical(log); };
        }
    }
}