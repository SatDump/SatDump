#pragma once

#include <nng/nng.h>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/util/platform.h>

#include <functional>
#include <string>

// Webserver for stats
namespace webserver
{
    extern std::function<std::string()> handle_callback;
    extern std::function<std::string(std::string)> handle_callback_html;
    extern std::function<std::vector<uint8_t>()> handle_callback_polarplot;
    extern std::function<std::vector<uint8_t>()> handle_callback_fft;
    extern std::function<std::vector<uint8_t>()> handle_callback_schedule;

    extern bool add_polarplot_handler;
    void start(std::string http_server_url);

    void stop();
};