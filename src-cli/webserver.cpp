#include "webserver.h"
#include <vector>
#include <mutex>

// Webserver for stats
namespace webserver
{
    /*TODOXP
    nng_http_server *http_server;
    nng_url *url;
    nng_http_handler *handler;
    nng_http_handler *handler_html;
    nng_http_handler *handler_polarplot;
    nng_http_handler *handler_fft;
    nng_http_handler *handler_schedule;
    */

    bool is_active = false;

    std::mutex request_mutex;

    std::function<std::string()> handle_callback = []() -> std::string
    { return ""; };

    std::function<std::string(std::string)> handle_callback_html = [](std::string) -> std::string
    { return "Please use /api for JSON data"; };

    std::function<std::vector<uint8_t>()> handle_callback_polarplot = []() -> std::vector<uint8_t>
    { return {}; };

    std::function<std::vector<uint8_t>()> handle_callback_fft = []() -> std::vector<uint8_t>
    { return {}; };

    std::function<std::vector<uint8_t>()> handle_callback_schedule = []() -> std::vector<uint8_t>
    { return {}; };

    // HTTP Handler for stats
    /*TODOXP
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
    */

    // HTTP Handler for HTML
    /* TODOXP
    void http_handle_html(nng_aio *aio)
    {
        request_mutex.lock();

        std::string uri = nng_http_req_get_uri((nng_http_req *)nng_aio_get_input(aio, 0));

        std::string jsonstr = handle_callback_html(uri);

        nng_http_res *res;
        nng_http_res_alloc(&res);
        nng_http_res_copy_data(res, jsonstr.c_str(), jsonstr.size());
        nng_http_res_set_header(res, "Content-Type", "text/html; charset=utf-8");
        nng_aio_set_output(aio, 0, res);
        nng_aio_finish(aio, 0);
        request_mutex.unlock();
    }
    */

    bool add_polarplot_handler = false;

    // HTTP Handler for HTML
    /*TODOXP
    void http_handle_polarplot(nng_aio *aio)
    {
        request_mutex.lock();

        std::vector<uint8_t> img = handle_callback_polarplot();

        nng_http_res *res;
        nng_http_res_alloc(&res);
        nng_http_res_copy_data(res, img.data(), img.size());
        nng_http_res_set_header(res, "Content-Type", "image/jpeg");
        nng_aio_set_output(aio, 0, res);
        nng_aio_finish(aio, 0);
        request_mutex.unlock();
    }
    */

    // HTTP Handler for HTML
    /*TODOXP
    void http_handle_fft(nng_aio *aio)
    {
        request_mutex.lock();

        std::vector<uint8_t> img = handle_callback_fft();

        nng_http_res *res;
        nng_http_res_alloc(&res);
        nng_http_res_copy_data(res, img.data(), img.size());
        nng_http_res_set_header(res, "Content-Type", "image/jpeg");
        nng_aio_set_output(aio, 0, res);
        nng_aio_finish(aio, 0);
        request_mutex.unlock();
    }
    */

    // HTTP Handler for Schedule
    /*TODOXP
    void http_handle_schedule(nng_aio *aio)
    {
        request_mutex.lock();

        std::vector<uint8_t> img = handle_callback_schedule();

        nng_http_res *res;
        nng_http_res_alloc(&res);
        nng_http_res_copy_data(res, img.data(), img.size());
        nng_http_res_set_header(res, "Content-Type", "image/jpeg");
        nng_aio_set_output(aio, 0, res);
        nng_aio_finish(aio, 0);
        request_mutex.unlock();
    }
    */

    void start(std::string http_server_url)
    {
        /*TODOXP
        http_server_url = "http://" + http_server_url;
        nng_url_parse(&url, http_server_url.c_str());
        nng_http_server_hold(&http_server, url);

        nng_http_handler_alloc(&handler, "/api", http_handle);
        nng_http_handler_set_method(handler, "GET");
        nng_http_server_add_handler(http_server, handler);

        nng_http_handler_alloc(&handler_html, "", http_handle_html);
        nng_http_handler_set_method(handler_html, "GET");
        nng_http_handler_set_tree(handler_html);
        nng_http_server_add_handler(http_server, handler_html);

        if (add_polarplot_handler)
        {
            nng_http_handler_alloc(&handler_polarplot, "/polarplot.jpeg", http_handle_polarplot);
            nng_http_handler_set_method(handler_polarplot, "GET");
            nng_http_server_add_handler(http_server, handler_polarplot);

            nng_http_handler_alloc(&handler_fft, "/fft.jpeg", http_handle_fft);
            nng_http_handler_set_method(handler_fft, "GET");
            nng_http_server_add_handler(http_server, handler_fft);

            nng_http_handler_alloc(&handler_schedule, "/schedule.jpeg", http_handle_schedule);
            nng_http_handler_set_method(handler_schedule, "GET");
            nng_http_server_add_handler(http_server, handler_schedule);
        }

        nng_http_server_start(http_server);
        nng_url_free(url);
        */
        is_active = true;
    }

    void stop()
    {
        if (is_active)
        {
            /*TODOXP
            nng_http_server_stop(http_server);
            nng_http_server_release(http_server);
            */
        }
    }
};
