#include <c_plugin_loader.h>

#include <chrono>
#include <cstring>
#include <string>
#include <vector>

#include <stdlib.h>

#define FRAME_SIZE 44356

#ifdef _MSC_VER
#include <intrin.h>
#define __builtin_popcount __popcnt
#endif

enum
{
    ZERO = 0,
    FORWARD = 0x33,
    BACKWARD = 0xCC
};

static int syncs[] = {ZERO, FORWARD, BACKWARD};

enum error_codes
{
    SUCCESS,
    ERR_OFFLINE_PROCESS
};

struct PluginData
{
    size_t buffer_length;
    std::vector<unsigned char> output_buffer;
    plugin_acquire_info_t acquire_info;

    int forward_counter;
    int zero_counter;
    int backward_counter;
    int now_lino;

    bool check_pn_sync_module;
    bool use_in_frame_counter;

    long long time_gap, symbol_gap;

    volatile long long *cross_module_shared_memory;

    bool use_time_counter;
    long long last_counter, now_counter;
    decltype(std::chrono::high_resolution_clock::now()) last_frame_time, now_time;
    int64_t milliseconds_elapsed, symbols_elapsed;

    int recv_lino_diff[10], recv_lino_diff_index;
};

const char *plugin_get_error_msg(plugin_error_t err)
{
    switch (err)
    {
    case ERR_OFFLINE_PROCESS:
        return "This plugin can only run in recorder\n";
    default:
        return "Unknown error";
    }
}

plugin_error_t plugin_acquire(plugin_acquire_info_t acquire_info, plugin_acquire_result_t *acquire_result)
{
    auto pdata = new PluginData;
    pdata->buffer_length = 0;
    pdata->acquire_info = acquire_info;

    acquire_result->buffer_size = FRAME_SIZE;
    acquire_result->plugin_custom_data = pdata;

    pdata->forward_counter = 0;
    pdata->zero_counter = 0;
    pdata->backward_counter = 0;
    pdata->now_lino = -1;
    pdata->recv_lino_diff_index = 0;
    pdata->last_counter = 0;
    pdata->now_counter = 0;
    pdata->symbol_gap = std::stoll(acquire_info.query_plugin_param(acquire_info.plugin_interface_data, "symbol_gap"));
    pdata->time_gap = std::stoll(acquire_info.query_plugin_param(acquire_info.plugin_interface_data, "time_gap"));

    pdata->check_pn_sync_module = false;

    if (pdata->acquire_info.live_process)
    {
        pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO,
                                      "Fix line numbers using interval");
    }
    else
    {
        pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, WARNING,
                                      "Offline process detected. Use in-frame counter.");
        pdata->check_pn_sync_module = true;
        pdata->use_time_counter = false;
        pdata->use_in_frame_counter = true;
    }

    return error_codes::SUCCESS;
}

int plugin_process_data(plugin_acquire_result_t *acquire_result, const unsigned char *data_arrived, size_t data_length,
                        unsigned char **output_pointer)
{
    PluginData *pdata = (PluginData *)(acquire_result->plugin_custom_data);

    if (!pdata->check_pn_sync_module)
    {
        pdata->cross_module_shared_memory = (volatile long long *)pdata->acquire_info.acquire_space_by_name(
            pdata->acquire_info.plugin_interface_data, "fengyun2_pn_sync_symbol_counter", 0);

        if (pdata->cross_module_shared_memory)
        {
            pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO,
                                          "using counters from pn_sync module");
            pdata->use_time_counter = false;
            pdata->use_in_frame_counter = false;

            if (pdata->cross_module_shared_memory[1])
            {
                pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO,
                                              "pn sync detected, uses in-frame counter");
                pdata->use_in_frame_counter = true;
            }
        }
        else
        {
            pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO,
                                          "pn_sync module not found, fallback to time counter");
            pdata->use_time_counter = true;
            pdata->use_in_frame_counter = false;
        }

        pdata->check_pn_sync_module = true;
    }

    pdata->output_buffer.resize(FRAME_SIZE);
    memcpy(pdata->output_buffer.data(), data_arrived, data_length);
    auto frame = pdata->output_buffer.data();

    if (pdata->use_time_counter)
    {
        pdata->now_time = std::chrono::high_resolution_clock::now();
        auto dur = pdata->now_time - pdata->last_frame_time;
        pdata->milliseconds_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
        pdata->last_frame_time = pdata->now_time;
    }
    else
    {
        if (pdata->use_in_frame_counter)
        {
            pdata->now_counter = *(long long *)(frame + 20);
            pdata->symbols_elapsed = pdata->now_counter - pdata->last_counter;
            pdata->last_counter = pdata->now_counter;
        }
        else
        {
            pdata->now_counter = pdata->cross_module_shared_memory[0];
            pdata->symbols_elapsed = pdata->now_counter - pdata->last_counter;
            pdata->last_counter = pdata->now_counter;
        }

        if (pdata->symbols_elapsed > pdata->symbol_gap * 2500)
            pdata->symbols_elapsed = 0;
    }

    int min_diff_bits = 32, closest_sync = 0;
    int sync = frame[3];

    for (int i = 0; i < 3; i++)
    {
        int now_sync = syncs[i];
        if (__builtin_popcount(sync ^ now_sync) < min_diff_bits)
        {
            min_diff_bits = __builtin_popcount(sync ^ now_sync);
            closest_sync = now_sync;
        }
    }

    frame[3] = closest_sync;

    // STATES:
    // ZERO: forward_counter = 0
    // FORWARD: forward_counter > 0
    // BACKWARD: forward_counter > 0, backward_counter > 5

    if (pdata->forward_counter == 0)
    {
        if (closest_sync == ZERO)
            pdata->zero_counter++;
        if (closest_sync == FORWARD)
        {
            if (pdata->zero_counter > 5)
            {
                pdata->forward_counter = 1;
                pdata->now_lino = 0;
                pdata->zero_counter = 0;

                for (auto &&d : pdata->recv_lino_diff)
                    d = 0;
                pdata->recv_lino_diff_index = 0;
                pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO, "Image detected");
            }
            else
            {
                pdata->zero_counter++;
            }
        }
    }
    else if (pdata->forward_counter > 0 && pdata->backward_counter < 5)
    {
        if (!pdata->use_time_counter)
        {
            int offset = pdata->symbols_elapsed / (double)pdata->symbol_gap + 0.5 - 1; // round
            if (offset > 0)
            {
                // pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO,
                //                               ("Counter added " + std::to_string(offset)).c_str());
                pdata->forward_counter += offset;
            }
        }
        else
        {
            int offset = pdata->milliseconds_elapsed / (double)pdata->time_gap + 0.5 - 1;
            if (offset > 0)
            {
                // pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO,
                //                               ("Counter added " + std::to_string(offset)).c_str());
                pdata->forward_counter += offset;
            }
        }

        pdata->now_lino = pdata->forward_counter++;

        if (closest_sync == FORWARD)
        {
            pdata->backward_counter = 0; // reset
            pdata->zero_counter = 0;
        }

        if (closest_sync == BACKWARD)
            pdata->backward_counter++;

        if (closest_sync == ZERO)
        {
            pdata->zero_counter++;

            if (pdata->zero_counter > 5)
            {
                pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO, "Image reset");
                pdata->forward_counter = 0;
                pdata->backward_counter = 0;
            }
        }

        frame[3] = FORWARD;
    }
    else
    {
        pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO, "Image stopped");
        // To zero state
        pdata->forward_counter = 0;
        pdata->zero_counter = 0;
        pdata->backward_counter = 0;
    }

    if (pdata->now_lino != -1)
    {
        // pdata->acquire_info.print_log(INFO,"lino updated: " + std::to_string(now_lino));
        frame[67] = pdata->now_lino >> 8;
        frame[68] = pdata->now_lino;
        pdata->now_lino = -1;
    }

    *output_pointer = pdata->output_buffer.data();

    return data_length;
}

void plugin_dispose(plugin_acquire_result_t *acquire_result)
{
    PluginData *pdata = (PluginData *)(acquire_result->plugin_custom_data);
    pdata->acquire_info.dispose_space_by_name(pdata->acquire_info.plugin_interface_data,
                                              "fengyun2_pn_sync_symbol_counter");
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
