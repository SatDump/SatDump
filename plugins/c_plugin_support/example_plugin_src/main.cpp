#include <c_plugin_loader.h>

#include <cstring>
#include <stdlib.h>
#include <string>
#include <vector>

enum error_codes
{
    SUCCESS,
    ERR_INVALID_BUFFER_LENGTH
};

struct PluginData
{
    size_t buffer_length;
    volatile long long *counter;
    std::vector<unsigned char> buffer;
    plugin_acquire_info_t acquire_info;
};

const char *plugin_get_error_msg(plugin_error_t err)
{
    switch (err)
    {
    case ERR_INVALID_BUFFER_LENGTH:
        return "Invalid buffer length";
    default:
        return "Unknown error";
    }
}

plugin_error_t plugin_acquire(plugin_acquire_info_t acquire_info, plugin_acquire_result_t *acquire_result)
{
    auto pdata = new PluginData;
    pdata->acquire_info = acquire_info;

    int value = std::stoi(acquire_info.query_plugin_param(acquire_info.plugin_interface_data, "size"));

    if (value <= 0)
        return error_codes::ERR_INVALID_BUFFER_LENGTH;

    pdata->counter = (volatile long long *)acquire_info.acquire_space_by_name(
        pdata->acquire_info.plugin_interface_data, "concat_c_plugin_counter", sizeof(long long));
    *(pdata->counter) = 0;

    acquire_result->buffer_size = value;
    acquire_result->plugin_custom_data = pdata;

    acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO, "plugin acquired");

    return error_codes::SUCCESS;
}

int plugin_process_data(plugin_acquire_result_t *acquire_result, const unsigned char *data_arrived, size_t data_length,
                        unsigned char **output_pointer)
{
    PluginData *pdata = (PluginData *)(acquire_result->plugin_custom_data);
    if (data_length > pdata->buffer.size())
    {
        pdata->buffer.resize(data_length);
    }

    *output_pointer = pdata->buffer.data();

    memcpy(pdata->buffer.data(), data_arrived, data_length);
    *(pdata->counter) += data_length;

    return data_length;
}

void plugin_dispose(plugin_acquire_result_t *acquire_result)
{
    PluginData *pdata = (PluginData *)(acquire_result->plugin_custom_data);
    pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO, "Plugin disposed");
    pdata->acquire_info.dispose_space_by_name(pdata->acquire_info.plugin_interface_data, "concat_c_plugin_counter");
    delete pdata;
}

extern "C"
{
    void plugin_query_interface(plugin_interface_t *interface)
    {
        interface->version = 1;
        interface->plugin_geterr = &plugin_get_error_msg;
        interface->plugin_acquire = &plugin_acquire;
        interface->plugin_process_data = &plugin_process_data;
        interface->plugin_dispose = &plugin_dispose;
        interface->draw_ui = nullptr;
    }
}
