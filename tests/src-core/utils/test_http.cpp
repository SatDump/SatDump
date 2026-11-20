#include "helpers/sockets.h"
#include "satdump_vars.h"
#include "utils/http.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <optional>
#include <string>
#include <thread>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

// Not declared in http.h, but exported by satdump_core
namespace satdump
{
    int curl_float_progress_func(void *ptr, curl_off_t TotalToDownload, curl_off_t NowDownloaded, curl_off_t TotalToUpload, curl_off_t NowUploaded);
}

// Requests only ever go to a one-shot HTTP server on 127.0.0.1 started by the test itself.
// That server uses POSIX sockets (and EnvGuard setenv/unsetenv), so the tests needing it are
// POSIX only.

#ifndef _WIN32
namespace
{
    using test_helpers::closedPort;

    // Sets or unsets an environment variable, restoring the previous value afterwards
    struct EnvGuard
    {
        std::string name;
        std::optional<std::string> old;
        EnvGuard(const std::string &n, const char *value) : name(n)
        {
            const char *o = getenv(n.c_str());
            if (o)
                old = o;
            if (value)
                setenv(n.c_str(), value, 1);
            else
                unsetenv(n.c_str());
        }
        ~EnvGuard()
        {
            if (old)
                setenv(name.c_str(), old->c_str(), 1);
            else
                unsetenv(name.c_str());
        }
    };

    // Make sure no proxy configured in the environment intercepts loopback requests
    struct NoProxy
    {
        EnvGuard a{"http_proxy", nullptr}, b{"HTTP_PROXY", nullptr}, c{"all_proxy", nullptr}, d{"ALL_PROXY", nullptr};
        EnvGuard e{"no_proxy", "*"}, f{"NO_PROXY", "*"};
    };

    // Accepts one connection on 127.0.0.1, records the request and answers with a fixed response.
    // Every blocking step is bounded by poll() so a failed request cannot hang the test.
    class OneShotHttpServer
    {
    private:
        int listen_fd = -1;
        std::thread th;
        std::string response;

        static bool waitReadable(int fd)
        {
            pollfd p{fd, POLLIN, 0};
            return poll(&p, 1, 15000) > 0;
        }

        void serve()
        {
            if (!waitReadable(listen_fd))
                return;
            int c = accept(listen_fd, nullptr, nullptr);
            if (c < 0)
                return;

            size_t header_end = std::string::npos;
            size_t content_length = 0;
            char buf[4096];
            while (true)
            {
                if (header_end != std::string::npos && request.size() >= header_end + 4 + content_length)
                    break;
                if (!waitReadable(c))
                    break;
                ssize_t n = recv(c, buf, sizeof(buf), 0);
                if (n <= 0)
                    break;
                request.append(buf, n);
                if (header_end == std::string::npos && (header_end = request.find("\r\n\r\n")) != std::string::npos)
                {
                    size_t cl = request.find("Content-Length: ");
                    if (cl != std::string::npos && cl < header_end)
                        content_length = std::stoul(request.substr(cl + 16));
                }
            }

            size_t sent = 0;
            while (sent < response.size())
            {
                ssize_t n = send(c, response.data() + sent, response.size() - sent, MSG_NOSIGNAL);
                if (n <= 0)
                    break;
                sent += n;
            }
            close(c);
        }

    public:
        int port = 0;
        std::string request;

        OneShotHttpServer(const std::string &status, const std::string &body)
        {
            response = "HTTP/1.1 " + status + "\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;

            listen_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (listen_fd < 0)
                throw std::runtime_error("socket() failed");
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = 0;
            socklen_t len = sizeof(addr);
            if (bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) != 0 || listen(listen_fd, 1) != 0 || getsockname(listen_fd, (sockaddr *)&addr, &len) != 0)
            {
                close(listen_fd);
                throw std::runtime_error("Could not set up loopback server");
            }
            port = ntohs(addr.sin_port);
            th = std::thread(&OneShotHttpServer::serve, this);
        }

        // Waits for the exchange to finish, so request is complete and safe to read
        void finish()
        {
            if (th.joinable())
                th.join();
        }

        ~OneShotHttpServer()
        {
            finish();
            close(listen_fd);
        }

        std::string url(const std::string &path) { return "http://127.0.0.1:" + std::to_string(port) + path; }
    };

    bool contains(const std::string &haystack, const std::string &needle) { return haystack.find(needle) != std::string::npos; }
} // namespace
#endif

TEST_CASE("curl_write_std_string appends the chunk and reports its size", "[utils][http]")
{
    std::string s = "ab";
    char data[] = "cdefgh";
    CHECK(satdump::curl_write_std_string(data, 2, 3, &s) == 6);
    CHECK(s == "abcdefgh");
    CHECK(satdump::curl_write_std_string(data, 1, 0, &s) == 0);
    CHECK(s == "abcdefgh");
}

TEST_CASE("curl_float_progress_func reports download, then upload, progress", "[utils][http]")
{
    float p = -1;
    CHECK(satdump::curl_float_progress_func(&p, 200, 50, 0, 0) == 0);
    CHECK(p == Catch::Approx(0.25f));

    // Download takes precedence when both are known
    CHECK(satdump::curl_float_progress_func(&p, 10, 10, 100, 1) == 0);
    CHECK(p == Catch::Approx(1.0f));

    CHECK(satdump::curl_float_progress_func(&p, 0, 0, 40, 30) == 0);
    CHECK(p == Catch::Approx(0.75f));

    // Unknown totals leave the value untouched
    p = 0.5f;
    CHECK(satdump::curl_float_progress_func(&p, 0, 0, 0, 0) == 0);
    CHECK(p == 0.5f);
}

#ifndef _WIN32
TEST_CASE("perform_http_request fetches a body over loopback", "[utils][http]")
{
    NoProxy np;
    OneShotHttpServer server("200 OK", "hello from the test server");
    std::string result;
    float progress = 0;
    int ret = satdump::perform_http_request(server.url("/some/path?x=1"), result, "X-Unit-Test: yes", &progress);
    server.finish();

    CHECK(ret == 0);
    CHECK(result == "hello from the test server");
    CHECK(progress > 0.0f);
    CHECK(progress <= 1.0f);

    CHECK(server.request.rfind("GET /some/path?x=1 HTTP/1.1\r\n", 0) == 0);
    CHECK(contains(server.request, "\r\nUser-Agent: SatDump/v" + satdump::SATDUMP_VERSION + "\r\n"));
    CHECK(contains(server.request, "\r\nX-Unit-Test: yes\r\n"));
}

TEST_CASE("perform_http_request appends to an existing result string", "[utils][http]")
{
    // The result is not cleared first, so callers reusing a string accumulate responses
    NoProxy np;
    OneShotHttpServer server("200 OK", "body");
    std::string result = "previous;";
    CHECK(satdump::perform_http_request(server.url("/"), result) == 0);
    CHECK(result == "previous;body");
}

TEST_CASE("perform_http_request does not treat HTTP error statuses as failures", "[utils][http]")
{
    // CURLOPT_FAILONERROR is not set, so a 404 still returns 0 along with the error page
    NoProxy np;
    OneShotHttpServer server("404 Not Found", "nothing here");
    std::string result;
    CHECK(satdump::perform_http_request(server.url("/missing"), result) == 0);
    CHECK(result == "nothing here");
}

TEST_CASE("perform_http_request fails on a refused connection", "[utils][http]")
{
    NoProxy np;
    std::string result;
    CHECK(satdump::perform_http_request("http://127.0.0.1:" + std::to_string(closedPort()) + "/", result) == 1);
    CHECK(result.empty());
}

#endif

TEST_CASE("perform_http_request fails on an unsupported URL scheme", "[utils][http]")
{
    std::string result;
    CHECK(satdump::perform_http_request("satdumpnoproto://127.0.0.1/x", result) == 1);
    CHECK(result.empty());
}

#ifndef _WIN32
TEST_CASE("perform_http_request_post sends the payload and headers", "[utils][http]")
{
    NoProxy np;
    OneShotHttpServer server("200 OK", "{\"ok\":true}");
    std::string result;
    int ret = satdump::perform_http_request_post(server.url("/api"), result, "a=1&b=two", "X-Post-Test: 42");
    server.finish();

    CHECK(ret == 0);
    CHECK(result == "{\"ok\":true}");
    CHECK(server.request.rfind("POST /api HTTP/1.1\r\n", 0) == 0);
    CHECK(contains(server.request, "\r\nUser-Agent: SatDump/v" + satdump::SATDUMP_VERSION + "\r\n"));
    CHECK(contains(server.request, "\r\nX-Post-Test: 42\r\n"));
    CHECK(contains(server.request, "\r\nContent-Length: 9\r\n"));
    CHECK(server.request.substr(server.request.size() - 9) == "a=1&b=two");
}

TEST_CASE("perform_http_request_post fails on a refused connection", "[utils][http]")
{
    NoProxy np;
    std::string result;
    CHECK(satdump::perform_http_request_post("http://127.0.0.1:" + std::to_string(closedPort()) + "/", result, "x=1") == 1);
    CHECK(result.empty());
}
#endif
