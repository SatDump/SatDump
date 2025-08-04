#define SATDUMP_DLL_EXPORT 1
#include "plugin.h"
#ifdef _WIN32
#include "libs/dlfcn/dlfcn.h"
#else
#include <dlfcn.h>
#endif
#include "core/exception.h"
#include "logger.h"
#include "satdump_vars.h"
#include <cstdlib>
#include <filesystem>

std::shared_ptr<satdump::Plugin> loadPlugin(std::string plugin)
{
    logger->trace("Loading plugin " + plugin + "...");

    void *dynlib = dlopen(plugin.c_str(), RTLD_LAZY);
    if (!dynlib)
        throw satdump_exception("Error loading " + plugin + "! Error : " + std::string(dlerror()));

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
    SATDUMP_DLL std::shared_ptr<EventBus> eventBus = std::make_shared<EventBus>();
    SATDUMP_DLL std::shared_ptr<TaskScheduler> taskScheduler = std::make_shared<TaskScheduler>();
}; // namespace satdump

#ifdef __ANDROID__
std::string android_plugins_dir = "";
#endif

void loadPlugins(std::map<std::string, std::shared_ptr<satdump::Plugin>> &loaded_plugins)
{
    // Get possible paths
    std::vector<std::string> plugins_paths;
#ifdef __ANDROID__
    plugins_paths.push_back(android_plugins_dir + "/");
#else
    if (std::filesystem::exists("plugins"))
        plugins_paths.push_back("./plugins");              // Try local plugins directory first
    plugins_paths.push_back(satdump::LIBPATH + "plugins"); // Followed by system
#endif

    // Get platform file extensions
#if defined(_WIN32)
    std::filesystem::path extension = ".dll";
#elif defined(__APPLE__)
    std::filesystem::path extension = ".dylib";
#else
    std::filesystem::path extension = ".so";
#endif

    // Load all plugins
    for (std::string &plugins_path : plugins_paths)
    {
        logger->info("Loading plugins from " + plugins_path);

#ifndef _WIN32
        setenv("LD_LIBRARY_PATH", plugins_path.c_str(), 1);
#endif

        std::filesystem::recursive_directory_iterator pluginIterator(plugins_path);
        std::error_code iteratorError;
        while (pluginIterator != std::filesystem::recursive_directory_iterator())
        {
            std::string path = pluginIterator->path().string();
            if (!std::filesystem::is_regular_file(pluginIterator->path()) || pluginIterator->path().extension() != extension)
                goto skip_this;

#ifdef __ANDROID__
            if (path.find("libandroid_imgui.so") != std::string::npos || path.find("libsatdump_core.so") != std::string::npos || path.find("libsatdump_interface.so") != std::string::npos)
                goto skip_this;
#endif
            try
            {
                std::shared_ptr<satdump::Plugin> pl = loadPlugin(path);
                loaded_plugins.insert({pl->getID(), pl});
            }
            catch (std::runtime_error &e)
            {
                logger->error(e.what());
            }

        skip_this:
            pluginIterator.increment(iteratorError);
            if (iteratorError)
                logger->critical(iteratorError.message());
        }

        if (loaded_plugins.size() > 0)
        {
            logger->debug("Loaded plugins (" + std::to_string(loaded_plugins.size()) + ") : ");
            for (std::pair<const std::string, std::shared_ptr<satdump::Plugin>> &it : loaded_plugins)
                logger->debug(" - " + it.first);
            break;
        }
        else
            logger->warn("No Plugins in " + plugins_path + "!");
    }
}
