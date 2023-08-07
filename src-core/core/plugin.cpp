#define SATDUMP_DLL_EXPORT 1
#include "plugin.h"
#ifdef _WIN32
#include "libs/dlfcn/dlfcn.h"
#else
#include <dlfcn.h>
#endif
#include "logger.h"
#include <filesystem>
#include "satdump_vars.h"

std::shared_ptr<satdump::Plugin> loadPlugin(std::string plugin)
{
    logger->trace("Loading plugin " + plugin + "...");

    void *dynlib = dlopen(plugin.c_str(), RTLD_LAZY);
    if (!dynlib)
        throw std::runtime_error("Error loading " + plugin + "! Error : " + std::string(dlerror()));

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
};

#ifdef __ANDROID__
std::string android_plugins_dir = "";
#endif

void loadPlugins(std::map<std::string, std::shared_ptr<satdump::Plugin>> &loaded_plugins)
{
#ifdef __ANDROID__
    // std::string plugins_path = satdump::LIBPATH + "plugins/" + (std::string)ANDROID_ABI_LIB;
    std::string plugins_path = android_plugins_dir + "/";
#else
    std::string plugins_path = satdump::LIBPATH + "plugins";
#endif

    if (std::filesystem::exists("plugins"))
#ifdef __ANDROID__
        ; // plugins_path = "plugins/" + (std::string)ANDROID_ABI_LIB;
#else
        plugins_path = "./plugins";
#endif

#if defined(_WIN32)
    std::string extension = ".dll";
#elif defined(__APPLE__)
    std::string extension = ".dylib";
#else
        std::string extension = ".so";
#endif

    logger->info("Loading plugins from " + plugins_path);

    std::filesystem::recursive_directory_iterator pluginIterator(plugins_path);
    std::error_code iteratorError;
    while (pluginIterator != std::filesystem::recursive_directory_iterator())
    {
        if (!std::filesystem::is_directory(pluginIterator->path()))
        {
            if (pluginIterator->path().filename().string().find(extension) != std::string::npos)
            {
                std::string path = pluginIterator->path().string();

                if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
                {
#if __ANDROID__
                    if (path.find("libandroid_imgui.so") != std::string::npos)
                        goto skip_this;
                    if (path.find("libsatdump_core.so") != std::string::npos)
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
                }
                else
                {
                    logger->error("File " + path + " is not a valid plugin!");
                }
            }
        }

#if __ANDROID__
    skip_this:
#endif
        pluginIterator.increment(iteratorError);
        if (iteratorError)
            logger->critical(iteratorError.message());
    }

    if (loaded_plugins.size() > 0)
    {
        logger->debug("Loaded plugins (" + std::to_string(loaded_plugins.size()) + ") : ");
        for (std::pair<const std::string, std::shared_ptr<satdump::Plugin>> &it : loaded_plugins)
            logger->debug(" - " + it.first);
    }
}