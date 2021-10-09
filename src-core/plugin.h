#pragma once

#include "dll_export.h"
#include <memory>
#include <map>
#include "libs/eventbus/event_bus.hpp"

#define PLUGIN_LOADER(constructor)                       \
    extern "C"                                           \
    {                                                    \
        satdump::Plugin *loader()                        \
        {                                                \
            return (satdump::Plugin *)new constructor(); \
        }                                                \
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
    SATDUMP_DLL extern std::shared_ptr<dp::event_bus> eventBus;
}; // namespace satdump

void loadPlugins();