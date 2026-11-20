#include "cli/cli_test_helpers.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <thread>

// The stats webserver of the live, record and autotrack commands. It is only ever started on a
// free 127.0.0.1 port and queried with the POSIX client of the helpers, so these tests are
// POSIX only. Every start() is paired with exactly one stop(): stopping twice is not safe (see
// tests/UNTESTED_ISSUES.md).

#ifndef _WIN32
using namespace cli_test;

namespace
{
    // Starts the webserver on a free loopback port, stops it on destruction
    struct RunningServer
    {
        int port;

        RunningServer() : port(test_helpers::closedPort()) { webserver::start(loopback(port)); }
        ~RunningServer() { webserver::stop(); }

        RunningServer(const RunningServer &) = delete;
        RunningServer &operator=(const RunningServer &) = delete;

        HttpResponse get(const std::string &path) { return httpRequest(port, path); }
    };

    const std::vector<uint8_t> POLAR = {0xFF, 0xD8, 0x00, 0x01, 0x02, 0xFF, 0xD9};
    const std::vector<uint8_t> FFT = {0xFF, 0xD8, 0x10, 0x00, 0x11, 0xFF, 0xD9};
    const std::vector<uint8_t> SCHEDULE = {0xFF, 0xD8, 0x20, 0x21, 0xFF, 0xD9};

    std::string str(const std::vector<uint8_t> &v) { return std::string(v.begin(), v.end()); }

    void setImageCallbacks()
    {
        webserver::handle_callback_polarplot = []() { return POLAR; };
        webserver::handle_callback_fft = []() { return FFT; };
        webserver::handle_callback_schedule = []() { return SCHEDULE; };
    }
} // namespace

TEST_CASE("webserver serves the stats callback as JSON on /api", "[cli][legacy][webserver]")
{
    WebserverGuard guard;
    std::atomic<int> calls{0};
    webserver::handle_callback = [&calls]()
    {
        calls++;
        return std::string("{\"calls\": ") + std::to_string(calls.load()) + "}";
    };

    RunningServer server;

    HttpResponse first = server.get("/api");
    REQUIRE(first.ok);
    CHECK(first.status == 200);
    CHECK(first.hasHeader("Content-Type: application/json; charset=utf-8"));
    CHECK(first.body == "{\"calls\": 1}");

    // The callback runs for every request, nothing is cached
    HttpResponse second = server.get("/api");
    REQUIRE(second.ok);
    CHECK(second.body == "{\"calls\": 2}");
    CHECK(calls == 2);
}

TEST_CASE("webserver serves the HTML callback on every other path", "[cli][legacy][webserver]")
{
    WebserverGuard guard;
    std::mutex mtx;
    std::vector<std::string> uris;
    webserver::handle_callback = []() { return std::string("api"); };
    webserver::handle_callback_html = [&](std::string uri)
    {
        std::lock_guard<std::mutex> l(mtx);
        uris.push_back(uri);
        return "<p>page for " + uri + "</p>";
    };

    RunningServer server;

    for (std::string path : {"/", "/status", "/deeper/path.html", "/status?r=12345"})
    {
        INFO("path " << path);
        HttpResponse res = server.get(path);
        REQUIRE(res.ok);
        CHECK(res.status == 200);
        CHECK(res.hasHeader("Content-Type: text/html; charset=utf-8"));
        CHECK(res.body == "<p>page for " + path + "</p>");
    }

    // The callback gets the URI as requested, query string included
    std::lock_guard<std::mutex> l(mtx);
    CHECK(uris == std::vector<std::string>{"/", "/status", "/deeper/path.html", "/status?r=12345"});
}

TEST_CASE("webserver default callbacks", "[cli][legacy][webserver]")
{
    // A guard is still needed: another test may have run before with its own callbacks
    // restored to these defaults, which is what is checked here
    WebserverGuard guard;
    RunningServer server;

    HttpResponse api = server.get("/api");
    REQUIRE(api.ok);
    CHECK(api.status == 200);
    CHECK(api.body == "");

    HttpResponse page = server.get("/");
    REQUIRE(page.ok);
    CHECK(page.body == "Please use /api for JSON data");
}

TEST_CASE("webserver only accepts GET", "[cli][legacy][webserver]")
{
    WebserverGuard guard;
    std::atomic<int> calls{0};
    webserver::handle_callback = [&calls]()
    {
        calls++;
        return std::string("{}");
    };

    RunningServer server;

    HttpResponse res = httpRequest(server.port, "/api", "POST");
    REQUIRE(res.ok);
    CHECK(res.status == 405);
    CHECK(calls == 0);
}

TEST_CASE("webserver image handlers exist only when add_polarplot_handler is set", "[cli][legacy][webserver]")
{
    WebserverGuard guard;
    setImageCallbacks();
    webserver::handle_callback_html = [](std::string uri) { return "html " + uri; };

    SECTION("enabled")
    {
        webserver::add_polarplot_handler = true;
        RunningServer server;

        HttpResponse polar = server.get("/polarplot.jpeg");
        REQUIRE(polar.ok);
        CHECK(polar.status == 200);
        CHECK(polar.hasHeader("Content-Type: image/jpeg"));
        CHECK(polar.body == str(POLAR));

        HttpResponse fft = server.get("/fft.jpeg");
        REQUIRE(fft.ok);
        CHECK(fft.hasHeader("Content-Type: image/jpeg"));
        CHECK(fft.body == str(FFT));

        HttpResponse schedule = server.get("/schedule.jpeg");
        REQUIRE(schedule.ok);
        CHECK(schedule.hasHeader("Content-Type: image/jpeg"));
        CHECK(schedule.body == str(SCHEDULE));

        // The other handlers are unaffected
        CHECK(server.get("/status").body == "html /status");
    }

    SECTION("disabled")
    {
        webserver::add_polarplot_handler = false;
        RunningServer server;

        // Falls through to the HTML handler
        HttpResponse polar = server.get("/polarplot.jpeg");
        REQUIRE(polar.ok);
        CHECK(polar.hasHeader("Content-Type: text/html; charset=utf-8"));
        CHECK(polar.body == "html /polarplot.jpeg");
        CHECK(server.get("/fft.jpeg").body == "html /fft.jpeg");
        CHECK(server.get("/schedule.jpeg").body == "html /schedule.jpeg");
    }
}

TEST_CASE("webserver serves binary bodies unchanged", "[cli][legacy][webserver]")
{
    WebserverGuard guard;
    webserver::add_polarplot_handler = true;

    // Every byte value, including NUL, several times over
    std::vector<uint8_t> blob(4096);
    for (size_t i = 0; i < blob.size(); i++)
        blob[i] = (uint8_t)(i * 7);
    webserver::handle_callback_polarplot = [&blob]() { return blob; };

    RunningServer server;
    HttpResponse res = server.get("/polarplot.jpeg");
    REQUIRE(res.ok);
    CHECK(res.hasHeader("Content-Length: 4096"));
    CHECK(res.body == str(blob));
}

TEST_CASE("webserver handles requests from several clients at once", "[cli][legacy][webserver]")
{
    WebserverGuard guard;

    // The handlers are serialised by a mutex: a non-atomic counter must not lose updates
    int counter = 0;
    webserver::handle_callback = [&counter]()
    {
        int v = counter;
        std::this_thread::yield();
        counter = v + 1;
        return std::to_string(counter);
    };

    RunningServer server;

    const int THREADS = 4, REQUESTS = 10;
    std::atomic<int> good{0};
    std::vector<std::thread> clients;
    for (int t = 0; t < THREADS; t++)
        clients.emplace_back(
            [&]()
            {
                for (int i = 0; i < REQUESTS; i++)
                {
                    HttpResponse res = httpRequest(server.port, "/api");
                    if (res.ok && res.status == 200)
                        good++;
                }
            });
    for (auto &c : clients)
        c.join();

    CHECK(good == THREADS * REQUESTS);
    CHECK(counter == THREADS * REQUESTS);
}

TEST_CASE("webserver stops listening on stop and can be started again", "[cli][legacy][webserver]")
{
    WebserverGuard guard;
    webserver::handle_callback = []() { return std::string("first"); };

    int port;
    {
        RunningServer server;
        port = server.port;
        CHECK(portIsOpen(port));
        CHECK(server.get("/api").body == "first");
    }
    CHECK_FALSE(portIsOpen(port));

    // Callbacks are read on every request, so they can be swapped between runs
    webserver::handle_callback = []() { return std::string("second"); };
    {
        RunningServer server;
        CHECK(server.get("/api").body == "second");
    }
}
#endif
