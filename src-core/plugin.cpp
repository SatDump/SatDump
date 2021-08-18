#define SATDUMP_DLL_EXPORT 1
#include "plugin.h"
#ifdef _WIN32
#include "common/dlfcn/dlfcn.h"
#else
#include <dlfcn.h>
#endif
#include "logger.h"
#include "settings.h"
#include <filesystem>

std::shared_ptr<satdump::Plugin> loadPlugin(std::string plugin)
{
    logger->trace("Loading plugin " + plugin + "...");

    void *dynlib = dlopen(plugin.c_str(), RTLD_LAZY);
    if (!dynlib)
        throw std::runtime_error("Error loading " + plugin + "!");

    void *create = dlsym(dynlib, "loader");
    const char *dlsym_error = dlerror();
    if (dlsym_error != NULL)
        logger->warn("Possible error loading symbols from plugin!");

    satdump::Plugin *pluginObject = reinterpret_cast<satdump::Plugin *(*)()>(create)();
    pluginObject->init();
    logger->trace("Plugin " + pluginObject->getID() + " loaded!");

    return std::shared_ptr<satdump::Plugin>(pluginObject);
}

namespace satdump
{
    SATDUMP_DLL std::map<std::string, std::shared_ptr<satdump::Plugin>> loaded_plugins;
    SATDUMP_DLL std::shared_ptr<dp::event_bus> eventBus = std::make_shared<dp::event_bus>();
};

void loadPlugins()
{
    if (global_cfg.contains("plugins"))
    {
        std::vector<std::string> pluginsToLoad = global_cfg["plugins"].get<std::vector<std::string>>();
        for (std::string path : pluginsToLoad)
        {
            if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
            {
                std::shared_ptr<satdump::Plugin> pl = loadPlugin(path);
                satdump::loaded_plugins.insert({pl->getID(), pl});
            }
            else
            {
                logger->error("File " + path + " is not a valid plugin!");
            }
        }

        if (satdump::loaded_plugins.size() > 0)
        {
            logger->debug("Loaded plugins (" + std::to_string(satdump::loaded_plugins.size()) + ") : ");
            for (std::pair<const std::string, std::shared_ptr<satdump::Plugin>> &it : satdump::loaded_plugins)
                logger->debug(" - " + it.first);
        }
    }
}