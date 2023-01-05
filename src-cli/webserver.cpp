#include "webserver.h"
#include <mutex>

// Webserver for stats
namespace webserver
{
    nng_http_server *http_server;
    nng_url *url;
    nng_http_handler *handler;

    bool is_active = false;

    std::mutex request_mutex;

    std::function<std::string()> handle_callback = []() -> std::string
    { return ""; };

    // HTTP Handler for stats
    void http_handle(nng_aio *aio)
    {
        request_mutex.lock();

        std::string jsonstr = handle_callback();

        nng_http_res *res;
        nng_http_res_alloc(&res);
        nng_http_res_copy_data(res, jsonstr.c_str(), jsonstr.size());
        nng_http_res_set_header(res, "Content-Type", "application/json; charset=utf-8");
        nng_aio_set_output(aio, 0, res);
        nng_aio_finish(aio, 0);
        request_mutex.unlock();
    }

    void start(std::string http_server_url)
    {
        http_server_url = "http://" + http_server_url;
        nng_url_parse(&url, http_server_url.c_str());
        nng_http_server_hold(&http_server, url);
        nng_http_handler_alloc(&handler, url->u_path, http_handle);
        nng_http_handler_set_method(handler, "GET");
        nng_http_server_add_handler(http_server, handler);
        nng_http_server_start(http_server);
        nng_url_free(url);
        is_active = true;
    }

    void stop()
    {
        if (is_active)
        {
            nng_http_server_stop(http_server);
            nng_http_server_release(http_server);
        }
    }
};