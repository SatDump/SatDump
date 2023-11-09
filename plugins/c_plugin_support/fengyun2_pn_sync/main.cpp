#include <c_plugin_loader.h>

#include <bitset>
#include <cstring>
#include <stdlib.h>
#include <string>
#include <vector>

#define BUFFER_SIZE 8192
#define LOCAL_BUFFER_SIZE (BUFFER_SIZE + 64)
#define TARGET_WROTE_BITS (354848)

namespace
{
enum error_codes
{
    SUCCESS
};

struct PluginData
{
    size_t buffer_length;
    int pn_sync_fix;

    // [0]: counter
    // [1]: syncs pn
    volatile long long *cross_module_shared_memory;

    int64_t bits_wrote;
    std::bitset<64> shifter, pn_shifter;

    int64_t pn_right_bit_counter;
    std::vector<unsigned char> buffer;

    uint64_t now_writing_timestamp;

    uint8_t last_read_bit, last_write_bit;

    plugin_acquire_info_t acquire_info;
};

std::bitset<64> markers("0100101110111011101110011001100110010101010101010111111111111111");

const char *plugin_get_error_msg(plugin_error_t err)
{
    switch (err)
    {
    default:
        return "Unknown error";
    }
}

plugin_error_t plugin_acquire(plugin_acquire_info_t acquire_info, plugin_acquire_result_t *acquire_result)
{
    auto pdata = new PluginData;
    pdata->acquire_info = acquire_info;

    pdata->cross_module_shared_memory = (volatile long long *)acquire_info.acquire_space_by_name(
        pdata->acquire_info.plugin_interface_data, "fengyun2_pn_sync_symbol_counter", sizeof(long long) * 2);
    pdata->cross_module_shared_memory[0] = 0;

    pdata->buffer.resize(LOCAL_BUFFER_SIZE);
    pdata->bits_wrote = -1;
    pdata->pn_sync_fix = std::stoi(acquire_info.query_plugin_param(acquire_info.plugin_interface_data, "pn_sync"));

    pdata->cross_module_shared_memory[1] = pdata->pn_sync_fix;
    pdata->pn_right_bit_counter = 0;

    acquire_result->buffer_size = BUFFER_SIZE;
    acquire_result->plugin_custom_data = pdata;

    if (pdata->pn_sync_fix)
        acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO, "Resync pn codes");

    acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO, "plugin acquired");

    return error_codes::SUCCESS;
}

int plugin_process_data(plugin_acquire_result_t *acquire_result, const unsigned char *data_arrived, size_t data_length,
                        unsigned char **output_pointer)
{
    const int8_t *signed_buffer = (const int8_t *)data_arrived;

    PluginData *pdata = (PluginData *)(acquire_result->plugin_custom_data);
    pdata->cross_module_shared_memory[0] += data_length;
    *output_pointer = pdata->buffer.data();

    if (!pdata->pn_sync_fix)
    {
        memcpy(pdata->buffer.data(), data_arrived, data_length);

        return data_length;
    }
    else
    {
        long long output_buffer_bit_index = 0;
        auto &pn_shifter = pdata->pn_shifter;
        auto &shifter = pdata->shifter;
        auto &pn_right_bit_counter = pdata->pn_right_bit_counter;
        auto &last_read_bit = pdata->last_read_bit;
        auto &last_write_bit = pdata->last_write_bit;

#define PUSHBIT(x, val)                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        uint8_t __bit_to_write = x ^ last_write_bit;                                                                   \
        last_write_bit = !!__bit_to_write;                                                                             \
        pdata->buffer[output_buffer_bit_index] = __bit_to_write ? val : -val;                                          \
        output_buffer_bit_index++;                                                                                     \
    } while (0)

        for (int i = 0; i < data_length; i++)
        {
            uint8_t now_bit = signed_buffer[i] > 0;
            uint8_t bit = last_read_bit ^ now_bit;
            last_read_bit = now_bit;
            int8_t value = abs(signed_buffer[i]);

            if (value == 0)
                value = 1;

            uint8_t pn_bit = pn_shifter[13] ^ pn_shifter[14];

            shifter <<= 1;
            shifter |= bit;

            pn_shifter <<= 1;
            pn_shifter |= pn_bit;

            if (pdata->bits_wrote >= 0)
            {
                uint8_t byte_mask = (pdata->bits_wrote / 8) & 1;
                if (20 * 8 <= pdata->bits_wrote && pdata->bits_wrote < (20 + 8) * 8)
                {
                    int byte_ind = (pdata->bits_wrote - 20 * 8) / 8;
                    int bit_ind = (pdata->bits_wrote - 20 * 8) % 8;
                    uint8_t bit_to_write = (((char *)&(pdata->now_writing_timestamp))[byte_ind] >> (7 - bit_ind)) & 1;

                    PUSHBIT(bit_to_write ^ pn_bit ^ byte_mask, value);
                }
                else
                {
                    PUSHBIT(bit, value);
                }
                pdata->bits_wrote++;

                if (pdata->bits_wrote == TARGET_WROTE_BITS)
                {
                    pdata->bits_wrote = -1;
                    pn_right_bit_counter = 0;
                }

                continue;
            }
            else
            {
                if (bit == pn_bit)
                {
                    if (pn_right_bit_counter < 5000)
                        pn_right_bit_counter++;

                    // if (pn_right_bit_counter == 1000)
                    // {
                    //     pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO,
                    //                                     "Locked %lld", pdata->cross_module_shared_memory[0]);
                    // }
                }
                else
                {
                    pn_right_bit_counter -= 2;
                    if (pn_right_bit_counter <= 0)
                    {
                        pn_right_bit_counter = 1;
                        pn_shifter = shifter;
                    }
                }

                PUSHBIT(0, value);

                if ((markers ^ pn_shifter).count() == 0)
                {
                    if (pn_right_bit_counter > 1000)
                    {
                        pdata->bits_wrote = 0;

                        for (int k = 63; k >= 0; k--)
                            PUSHBIT(markers[k], value);

                        pdata->now_writing_timestamp = pdata->cross_module_shared_memory[0];
                    }
                }
            }
        }

        return output_buffer_bit_index;
    }
}

void plugin_dispose(plugin_acquire_result_t *acquire_result)
{
    PluginData *pdata = (PluginData *)(acquire_result->plugin_custom_data);
    pdata->acquire_info.print_log(pdata->acquire_info.plugin_interface_data, INFO, "Plugin disposed");
    pdata->acquire_info.dispose_space_by_name(pdata->acquire_info.plugin_interface_data,
                                              "fengyun2_pn_sync_symbol_counter");
    delete pdata;
}
} // namespace

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
