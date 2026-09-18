#pragma once

#include "dll_export.h"
#include <memory>
#include <map>

#include "utils/event_bus.h"
#include "utils/task_scheduler.h"

#define PLUGIN_ABI_VERSION 1

#define PLUGIN_LOADER(constructor)                                 \
    extern "C"                                                     \
    {                                                              \
        satdump::Plugin *loader()                                  \
        {                                                          \
            return (satdump::Plugin *)new constructor();           \
        }                                                          \
                                                                   \
        const extern int SATDUMP_ABI_VERSION = PLUGIN_ABI_VERSION; \
    }

namespace satdump
{
    struct SatDumpStartedEvent
    {
    };

    class Plugin
    {
    public:
        Plugin() {}
        virtual std::string getID() = 0;
        virtual void init() = 0;
        virtual ~Plugin(){};
    };

    SATDUMP_DLL extern std::map<std::string, std::shared_ptr<satdump::Plugin>> loaded_plugins;
    SATDUMP_DLL extern std::shared_ptr<EventBus> eventBus;
    SATDUMP_DLL extern std::shared_ptr<TaskScheduler> taskScheduler;
}; // namespace satdump

void loadPlugins(std::map<std::string, std::shared_ptr<satdump::Plugin>> &loaded_plugins = satdump::loaded_plugins);