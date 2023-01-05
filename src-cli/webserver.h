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

    // HTTP Handler for stats
    void http_handle(nng_aio *aio);

    void start(std::string http_server_url);

    void stop();
};