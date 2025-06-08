#pragma once

#include "core/exception.h"
#include "core/plugin.h"
#include "dll_export.h"
#include "nlohmann/json.hpp"
#include <functional>

namespace satdump
{
    namespace config
    {
        struct PluginConfigHandler
        {
            std::string name;
            std::function<void()> render;
            std::function<void()> save;
        };

        struct RegisterPluginConfigHandlersEvent
        {
            std::vector<PluginConfigHandler> &plugin_config_handlers;
        };
    } // namespace config

    class SatDumpConfigHandler
    {
    public:                                                              // TODOREWORK make this private again
        nlohmann::ordered_json main_cfg;                                 // The actual config we should use, that includes user changes
        std::vector<config::PluginConfigHandler> plugin_config_handlers; // TODOREWORK handle UI rendering here!

    public:                                 // TODOREWORK make private again asap!
        nlohmann::ordered_json default_cfg; // The main system-wide satdump_cfg.json
        std::string user_cfg_path;

    private:
        void checkOutputDirs();

    public:
        void load(std::string path, std::string user_path);
        void loadUser(std::string user_path);
        void saveUser();

        void registerPlugins()
        {
            eventBus->fire_event<config::RegisterPluginConfigHandlersEvent>({plugin_config_handlers});
            // TODOREWORK redo plugin config system to integrate them?
        }

    public:
        template <typename T>
        inline void tryAssignValueFromSatDumpGeneral(T &val, std::string variablename)
        {
            if (main_cfg["satdump_general"][variablename]["value"].is_null())
                return;
            val = main_cfg["satdump_general"][variablename]["value"].get<T>();
        }

        template <typename T>
        inline T getValueFromSatDumpGeneral(std::string variablename)
        {
            if (main_cfg["satdump_general"][variablename]["value"].is_null())
                throw satdump_exception("Config key " + variablename + " is missing!");
            return main_cfg["satdump_general"][variablename]["value"].get<T>();
        }

        template <typename T>
        inline T getValueFromSatDumpDirectories(std::string variablename)
        {
            if (main_cfg["satdump_directories"][variablename]["value"].is_null())
                throw satdump_exception("Config key " + variablename + " is missing!");
            return main_cfg["satdump_directories"][variablename]["value"].get<T>();
        }

        template <typename T>
        inline T getValueFromSatDumpUI(std::string variablename)
        {
            if (main_cfg["user_interface"][variablename]["value"].is_null())
                throw satdump_exception("Config key " + variablename + " is missing!");
            return main_cfg["user_interface"][variablename]["value"].get<T>();
        }

        // TODOREWORK have a set_user / get_user?

    public:
        bool shouldAutoprocessProducts() { return getValueFromSatDumpGeneral<bool>("auto_process_products"); } // TODOREWORK remove since per pipeline?
        bool shouldPlayAudio() { return getValueFromSatDumpUI<bool>("play_audio"); }
    };

    SATDUMP_DLL extern SatDumpConfigHandler satdump_cfg; // SatDump configuratiin
} // namespace satdump
