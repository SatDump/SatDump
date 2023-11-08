#pragma once

#include <stdlib.h>

typedef int plugin_error_t; // Type for representing errors returned by the plugin functions

enum logger_level
{
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// Struct type to hold initialization information for the plugin
typedef struct plugin_acquire_info
{
    int version; // Version of the plugin initialization information

    // Version 1

    // Data to be used as the first argument for most interface apis
    void *plugin_interface_data;

    // Function pointer to query plugin parameters
    // The plugin loader will provide a callback function that the plugin can use to obtain parameters.
    // Parameters:
    //   - data: A pointer to the data used to request the parameter.
    //   - param_name: The name of the parameter being queried.
    // Return:
    //   - A string managed by callee, containing the value of the queried param.
    //     NULL indicates param not exist.
    const char* (*query_plugin_param)(void *data, const char *param_name);
    
    // Function pointer to print log messages
    // The plugin can use this function to print log messages at different levels.
    // Parameters:
    //   - level: The severity level of the log message (DEBUG, INFO, WARNING, ERROR).
    //   - fmt: The format message to be printed in the log.
    void (*print_log)(void *data, int level, const char *fmt, ...);

    // Function pointer to acquire space by name
    // The plugin can use this function to acquire memory space by name and size.
    // Parameters:
    //   - name: The name of the space to be acquired.
    //   - size: The size of the memory space to be allocated.
    //           Zero size means do not alloc when not exists.
    //           Fail when required size is larger than the existing one.
    // Return:
    //   - A pointer to the acquired memory space if successful; otherwise, NULL.
    void *(*acquire_space_by_name)(void *data, const char *name, size_t size);

    // Function pointer to dispose of space by name
    // The plugin can use this function to release memory space acquired by name.
    // Parameters:
    //   - name: The name of the space to be disposed of.
    void (*dispose_space_by_name)(void *data, const char *name);

    // Non-zero means live processing
    int live_process;

    // Version 2

    // TODO: UI

} plugin_acquire_info_t;

typedef struct plugin_acquire_result
{
    int version; // Version of the plugin acquire result

    // Version 1

    size_t buffer_size;       // Size of the buffer
    void *plugin_custom_data; // Custom data related to the acquired plugin

    // ...
} plugin_acquire_result_t;

typedef struct plugin_interface
{
    int version; // Version of the plugin interface

    // Version 1

    // Function to retrieve error messages based on error codes
    // Parameter:
    //   - errno: The error code
    // Return:
    //   - A string message explaining the error
    const char *(*plugin_geterr)(plugin_error_t err);

    // Function to acquire an instance of the plugin based on provided information
    // Parameters:
    //   - acquire_info: Information needed to acquire the plugin
    //   - acquire_result: A pointer to store the acquire result
    // Return:
    //   - An error code indicating success or failure
    plugin_error_t (*plugin_acquire)(plugin_acquire_info_t acquire_info, plugin_acquire_result_t *acquire_result);

    // Function to process data using the acquired plugin instance
    // Parameters:
    //   - acquire_result: The plugin acquire result
    //   - data_arrived: Input data to be processed
    //   - data_length: Length of the input data; a zero length means EOF has been encountered
    //   - output_pointer: Pointer to store the processed data
    // Return:
    //   - An integer representing the result of the data processing
    //   - A positive or zero value signifies success. The value represents the number of bytes written
    //   - A negative value indicates an error. The value is the opposite number of errno
    int (*plugin_process_data)(plugin_acquire_result_t *acquire_result, const unsigned char *data_arrived,
                               size_t data_length, unsigned char **output_pointer);

    // Same as ProcessingModule::drawUI
    // ImGUI uses a subset with stable-ABI of C++ in its interface, so it's safe to expose it directly
    // NULL value means this plugin does not have UI
    void (*draw_ui)(int window);

    // Function to dispose of the acquired plugin instance
    // Parameter:
    //   - acquire_result: The plugin acquire result to be disposed of
    void (*plugin_dispose)(plugin_acquire_result_t *acquire_result);

    // ...
} plugin_interface_t;

// Plugins should export this interface as the symbol "plugin_query_interface"
typedef void plugin_query_interface_t(plugin_interface_t *interface);
