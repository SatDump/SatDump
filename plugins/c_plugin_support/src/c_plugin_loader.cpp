#include "c_plugin_loader.hpp"
#include "c_plugin_loader.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "resources.h"
#include "satdump_vars.h"

#include <atomic>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include "libs/dlfcn/dlfcn.h"
#else
#include <dlfcn.h>
#endif

namespace c_plugin
{

struct CPluginSharedSpace
{
    std::atomic_int64_t ref;
    std::vector<unsigned char> data;
};

static std::mutex shared_space_lock;
static std::map<std::string, CPluginSharedSpace> c_plugin_shared_space;

static void *acquire_space_by_name(void *, const char *name, size_t size)
{
    std::string key = name;

    std::lock_guard lock{shared_space_lock};

    if (auto it = c_plugin_shared_space.find(key); it != c_plugin_shared_space.end())
    {
        it->second.ref++;

        if (size > it->second.data.size())
            return NULL;

        return it->second.data.data();
    }
    else if (size > 0)
    {
        auto &&space = c_plugin_shared_space[key];
        space.ref = 1;
        space.data.resize(size);

        return space.data.data();
    }
    else
        return nullptr;
}

static void dispose_space_by_name(void *, const char *name)
{
    std::string key = name;

    std::lock_guard lock{shared_space_lock};

    if (auto it = c_plugin_shared_space.find(key); it != c_plugin_shared_space.end())
    {
        it->second.ref--;

        if (it->second.ref == 0)
            c_plugin_shared_space.erase(name);
    }
}

static void print_log(void *, int level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    switch (level)
    {
    case logger_level::DEBUG:
        logger->logf(slog::LOG_DEBUG, fmt, args);
        break;
    case logger_level::INFO:
        logger->logf(slog::LOG_INFO, fmt, args);
        break;
    case logger_level::WARNING:
        logger->logf(slog::LOG_WARN, fmt, args);
        break;
    case logger_level::ERROR:
        logger->logf(slog::LOG_ERROR, fmt, args);
        break;
    default:
        break;
    }
}

CPluginLoader::CPluginLoader(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    : ProcessingModule(input_file, output_file_hint, parameters),
      plugin_path(parameters["plugin_path"].template get<std::string>()), params(parameters["plugin_params"])
{
    std::string real_plugin_path = resources::getResourcePath(std::string("c_plugin") + "/" + plugin_path);

    dynlib = dlopen(real_plugin_path.c_str(), RTLD_LAZY);
    if (!dynlib)
        throw std::runtime_error("Error loading " + real_plugin_path + "! Error : " + std::string(dlerror()));

    plugin_query_interface_t *query_interface =
        reinterpret_cast<plugin_query_interface_t *>(dlsym(dynlib, "plugin_query_interface"));

    if (!query_interface)
    {
        throw std::runtime_error("Error loading " + real_plugin_path + "! Error : missing export functions");
    }

    query_interface(&plugin_interface);
}

CPluginLoader::~CPluginLoader()
{
    if (dynlib)
        dlclose(dynlib);
}

const char *CPluginLoader::query_plugin_param(void *loader, const char *param_name)
{
    thread_local std::string ret;
    CPluginLoader *plugin_loader = reinterpret_cast<CPluginLoader *>(loader);
    if (plugin_loader->params.contains(param_name))
    {
        ret = plugin_loader->params[param_name].template get<std::string>();

        return ret.c_str();
    }

    return nullptr;
}

std::vector<ModuleDataType> CPluginLoader::getInputTypes()
{
    return {DATA_FILE, DATA_STREAM};
}

std::vector<ModuleDataType> CPluginLoader::getOutputTypes()
{
    return {DATA_FILE, DATA_STREAM};
}

void CPluginLoader::process()
{
    unsigned char *output_buffer = nullptr;
    if (input_data_type == DATA_FILE)
        data_in = std::ifstream(d_input_file, std::ios::binary);
    if (output_data_type == DATA_FILE)
    {
        // Overwrite detection?
        std::string filename = d_output_file_hint + ".dat";

        for (int i = 0; std::filesystem::exists(filename); i++)
            filename = d_output_file_hint + "." + std::to_string(i) + ".dat";

        data_out = std::ofstream(filename, std::ios::binary);
        d_output_files.push_back(filename);
    }

    plugin_acquire_info_t plugin_acquire_info;
    plugin_acquire_info.version = 1;
    plugin_acquire_info.print_log = &print_log;
    plugin_acquire_info.query_plugin_param = &CPluginLoader::query_plugin_param;
    plugin_acquire_info.plugin_interface_data = this;
    plugin_acquire_info.acquire_space_by_name = &acquire_space_by_name;
    plugin_acquire_info.dispose_space_by_name = &dispose_space_by_name;
    plugin_acquire_info.live_process = input_data_type != DATA_FILE;

    plugin_acquire_result_t acquire_result;

    if (int err = plugin_interface.plugin_acquire(plugin_acquire_info, &acquire_result))
    {
        throw std::runtime_error(std::string("Plugin acquire error: ") + plugin_interface.plugin_geterr(err));
    }

    std::vector<char> buffer_container(acquire_result.buffer_size);

    std::shared_ptr<void> plugin_dispose_defer(
        nullptr, [&acquire_result, this](...) { this->plugin_interface.plugin_dispose(&acquire_result); });

    while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
    {
        // Read a buffer
        std::streamsize count = 0;
        if (input_data_type == DATA_FILE)
        {
            data_in.read((char *)buffer_container.data(), buffer_container.size());
            count = data_in.gcount();
        }
        else
            count = input_fifo->read((uint8_t *)buffer_container.data(), buffer_container.size());

        if (count > 0)
        {
            int ret = plugin_interface.plugin_process_data(
                &acquire_result, reinterpret_cast<unsigned char *>(buffer_container.data()), count, &output_buffer);

            if (ret < 0)
            {
                const char *error_message = plugin_interface.plugin_geterr(-ret);
                throw std::runtime_error(std::string("Plugin runtime error: ") + error_message);
            }

            if (ret > 0)
            {
                if (output_data_type == DATA_FILE)
                    data_out.write((char *)output_buffer, ret);
                else
                    output_fifo->write(output_buffer, ret);
            }
        }
    }

    int ret = plugin_interface.plugin_process_data(
        &acquire_result, reinterpret_cast<unsigned char *>(buffer_container.data()), 0, &output_buffer);

    if (ret < 0)
    {
        const char *error_message = plugin_interface.plugin_geterr(-ret);
        throw std::runtime_error(std::string("Plugin runtime error: ") + error_message);
    }

    if (ret > 0)
    {
        if (output_data_type == DATA_FILE)
            data_out.write((char *)output_buffer, ret);
        else
            output_fifo->write(output_buffer, ret);
    }

    if (output_data_type == DATA_FILE)
        data_out.close();
    if (input_data_type == DATA_FILE)
        data_in.close();
}

void CPluginLoader::drawUI(bool window)
{
    if (plugin_interface.draw_ui)
        plugin_interface.draw_ui(window);
}

bool CPluginLoader::hasUI()
{
    return plugin_interface.draw_ui != nullptr;
}

std::string CPluginLoader::getID()
{
    return "c_plugin_decoder";
}

std::vector<std::string> CPluginLoader::getParameters()
{
    return {"plugin_path", "plugin_params"};
}

std::shared_ptr<ProcessingModule> CPluginLoader::getInstance(std::string input_file, std::string output_file_hint,
                                                             nlohmann::json parameters)
{
    return std::make_shared<CPluginLoader>(input_file, output_file_hint, parameters);
}
} // namespace c_plugin
