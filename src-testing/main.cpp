/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"

#include <nng/nng.h>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/util/platform.h>
#include <nng/protocol/bus0/bus.h>
#include <nng/protocol/pair0/pair.h>

#include <unistd.h>

// HTTP Handler for stats
void http_handle(nng_aio *aio)
{
    std::string jsonstr = "{\"api\": true}";

    nng_http_res *res;
    nng_http_res_alloc(&res);
    nng_http_res_copy_data(res, jsonstr.c_str(), jsonstr.size());
    nng_http_res_set_header(res, "Content-Type", "application/json; charset=utf-8");
    nng_aio_set_output(aio, 0, res);
    nng_aio_finish(aio, 0);
}

int main(int argc, char *argv[])
{
    initLogger();

    std::string http_server_url = "http://0.0.0.0:8080";

    nng_http_server *http_server;
    nng_url *url;
    {

        nng_url_parse(&url, http_server_url.c_str());
        nng_http_server_hold(&http_server, url);
    }

    nng_http_handler *handler_api;
    {
        nng_http_handler_alloc(&handler_api, "/api", http_handle);
        nng_http_handler_set_method(handler_api, "GET");
        nng_http_server_add_handler(http_server, handler_api);
    }

    nng_socket socket;

    {

        int rv = 0;
        if (rv = nng_bus0_open(&socket); rv != 0)
        {
            printf("pair open error\n");
        }

        if (rv = nng_listen(socket, "ws://0.0.0.0:8080/ws", nullptr, 0); rv != 0)
        {
            printf("server listen error\n");
        }
    }

    nng_url_free(url);

    nng_http_server_start(http_server);

    while (1)
    {
        char *buf = nullptr;
        size_t size;

        int rv = 0;
        printf("WAIT\n");
        if (rv = nng_recv(socket, &buf, &size, NNG_FLAG_ALLOC); rv != 0)
        {
            printf("recv error: %s\n", nng_strerror(rv));
        }

        printf("server get with client: %s\n", buf);

        std::string testStr = "Server Reply!\n\r";
        //  int rv = 0;
        if (rv = nng_send(socket, (void *)testStr.c_str(), testStr.size(), NULL); rv != 0)
        {
            printf("send error: %s\n", nng_strerror(rv));
        }

        printf("SEND\n");

        sleep(1);
    }

    nng_http_server_stop(http_server);
    nng_http_server_release(http_server);
}