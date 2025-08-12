#include <algorithm>
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
    if (std::filesystem::exists("plugins")) // Try local plugins directory first
        plugins_paths.push_back("./plugins");
    if (std::filesystem::exists(satdump::LIBPATH + "plugins")) // Followed by system
        plugins_paths.push_back(satdump::LIBPATH + "plugins");
#endif

    if (plugins_paths.size() == 0)
        logger->critical("No valid plugin directory found!");

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

        std::vector<std::string> failed_plugins;
        std::vector<std::string> already_loaded_plugins;

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

            if (std::find_if(already_loaded_plugins.begin(), already_loaded_plugins.end(), [&](auto &v1) { return v1 == std::filesystem::path(path).stem().string(); }) != already_loaded_plugins.end())
            {
                logger->trace("Skipping duplicate plugin : " + std::filesystem::path(path).stem().string());
                goto skip_this;
            }

            try
            {
                std::shared_ptr<satdump::Plugin> pl = loadPlugin(path);
                loaded_plugins.insert({pl->getID(), pl});
                already_loaded_plugins.push_back(std::filesystem::path(path).stem().string());
            }
            catch (std::runtime_error &e)
            {
                failed_plugins.push_back(path);
            }

        skip_this:
            pluginIterator.increment(iteratorError);
            if (iteratorError)
                logger->critical(iteratorError.message());
        }

        // Try to reload failed ones, just in case. This allows handling dependencies to
        // some extent.
        int dependencies_layers = 0;
        const int max_dependencies_layers = 10;
        while (failed_plugins.size() && dependencies_layers < max_dependencies_layers)
        {
            std::vector<std::string> really_failed;
            for (auto &s : failed_plugins)
            {
                try
                {
                    std::shared_ptr<satdump::Plugin> pl = loadPlugin(s);
                    loaded_plugins.insert({pl->getID(), pl});
                }
                catch (std::runtime_error &e)
                {
                    if (dependencies_layers == (max_dependencies_layers - 1))
                        logger->error(e.what());
                    really_failed.push_back(s);
                }
            }
            failed_plugins = really_failed;
            dependencies_layers++;
        }

        if (loaded_plugins.size() > 0)
        {
            logger->debug("Loaded plugins (" + std::to_string(loaded_plugins.size()) + ") : ");
            for (std::pair<const std::string, std::shared_ptr<satdump::Plugin>> &it : loaded_plugins)
                logger->debug(" - " + it.first);
        }
        else
            logger->warn("No Plugins in " + plugins_path + "!");

        if (failed_plugins.size())
        {
            logger->warn("The following plugins failed to load :");
            for (auto &s : failed_plugins)
                logger->warn(" - " + std::filesystem::path(s).stem().string());
        }
    }
}
