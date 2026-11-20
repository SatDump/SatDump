#pragma once

// Shared helpers for the cli/ tests (src-cli). Everything lives in the
// cli_test namespace and is inline, so including this header from several
// translation units is fine.

#include "common/dsp/complex.h"
#include "common/dsp_source_sink/dsp_sample_sink.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "core/config.h"
#include "helpers/file_io.h"
#include "helpers/guards.h"
#include "helpers/satdump_guards.h"
#include "helpers/temp_files.h"
#include "legacy/webserver.h"
#include "logger.h"
#include <csignal>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#ifndef _WIN32
#include "helpers/sockets.h"
#include <fcntl.h>
#include <poll.h>
#endif

// Entry points and exit flags of the legacy commands that no header declares
int main_old(int argc, char *argv[]);
extern bool live_should_exit;
extern bool rec_should_exit;
extern bool autotrack_should_exit;

namespace cli_test
{
    using test_helpers::ConfigGuard;
    using test_helpers::EventBusGuard;
    using test_helpers::TempDir;
    using test_helpers::ValueGuard;

    // Builds a mutable argv from strings
    struct Argv
    {
        std::vector<std::string> storage;
        std::vector<char *> ptrs;

        Argv(std::vector<std::string> args) : storage(std::move(args))
        {
            for (auto &s : storage)
                ptrs.push_back(s.data());
            ptrs.push_back(nullptr);
        }

        Argv(const Argv &) = delete;
        Argv &operator=(const Argv &) = delete;

        int argc() { return (int)storage.size(); }
        char **argv() { return ptrs.data(); }
    };

    // Calls a legacy entry point (main_old, main_live...) with these arguments
    template <typename F>
    int call(F &&entry, std::vector<std::string> args)
    {
        Argv a(std::move(args));
        return entry(a.argc(), a.argv());
    }

    ///////////////////////////////////////////////////////////
    // Logging
    ///////////////////////////////////////////////////////////

    // Replaces the global logger with one that only records, and restores the previous one on
    // destruction. The legacy commands report nearly everything through the logger.
    class LogCapture
    {
    private:
        class Sink : public slog::LoggerSink
        {
        public:
            std::mutex mtx;
            std::vector<slog::LogMsg> msgs;

            void receive(slog::LogMsg log)
            {
                std::lock_guard<std::mutex> l(mtx);
                msgs.push_back(log);
            }
        };

        std::shared_ptr<slog::Logger> saved;
        std::shared_ptr<Sink> sink = std::make_shared<Sink>();

    public:
        LogCapture() : saved(logger)
        {
            logger = std::make_shared<slog::Logger>();
            logger->add_sink(sink);
            // add_sink replays the messages logged before completeLoggerInit(), which the test
            // main never calls
            std::lock_guard<std::mutex> l(sink->mtx);
            sink->msgs.clear();
        }

        ~LogCapture() { logger = saved; }

        LogCapture(const LogCapture &) = delete;
        LogCapture &operator=(const LogCapture &) = delete;

        // True if a message of this level (any level by default) contains text
        bool contains(const std::string &text, int lvl = -1)
        {
            std::lock_guard<std::mutex> l(sink->mtx);
            for (auto &m : sink->msgs)
                if ((lvl == -1 || m.lvl == lvl) && m.str.find(text) != std::string::npos)
                    return true;
            return false;
        }

        // Everything logged so far, one message per line (for INFO / CAPTURE)
        std::string dump()
        {
            std::lock_guard<std::mutex> l(sink->mtx);
            std::string out;
            for (auto &m : sink->msgs)
                out += m.str.c_str() + std::string("\n");
            return out;
        }
    };

    ///////////////////////////////////////////////////////////
    // Global state of the legacy commands
    ///////////////////////////////////////////////////////////

    // The commands install their own SIGINT / SIGTERM handlers and never remove them
    class SignalGuard
    {
    private:
        void (*saved_int)(int);
        void (*saved_term)(int);

    public:
        SignalGuard() : saved_int(std::signal(SIGINT, SIG_DFL)), saved_term(std::signal(SIGTERM, SIG_DFL))
        {
            std::signal(SIGINT, saved_int);
            std::signal(SIGTERM, saved_term);
        }

        ~SignalGuard()
        {
            std::signal(SIGINT, saved_int);
            std::signal(SIGTERM, saved_term);
        }

        SignalGuard(const SignalGuard &) = delete;
        SignalGuard &operator=(const SignalGuard &) = delete;
    };

    // Saves the webserver callbacks and settings, restores them on destruction. The commands
    // leave callbacks capturing their locals behind.
    struct WebserverGuard
    {
        ValueGuard<std::function<std::string()>> api{webserver::handle_callback};
        ValueGuard<std::function<std::string(std::string)>> html{webserver::handle_callback_html};
        ValueGuard<std::function<std::vector<uint8_t>()>> polarplot{webserver::handle_callback_polarplot};
        ValueGuard<std::function<std::vector<uint8_t>()>> fft{webserver::handle_callback_fft};
        ValueGuard<std::function<std::vector<uint8_t>()>> schedule{webserver::handle_callback_schedule};
        ValueGuard<bool> add_polarplot{webserver::add_polarplot_handler};
    };

    // Saves the source and sink registries, restores them on destruction
    struct DeviceRegistryGuard
    {
        ValueGuard<std::map<std::string, dsp::RegisteredSource>> sources{dsp::dsp_sources_registry};
        ValueGuard<std::map<std::string, dsp::RegisteredSink>> sinks{dsp::dsp_sinks_registry};
    };

    // Everything a command that opens the "file" source needs: the config key the file source
    // reads, an event bus without plugins (so "file" is the only source), and all the global
    // state the commands change put back afterwards. Exit flags start cleared.
    struct CommandEnv
    {
        ConfigGuard cfg;
        EventBusGuard bus;
        DeviceRegistryGuard devices;
        SignalGuard signals;
        WebserverGuard web;
        ValueGuard<bool> live_exit{live_should_exit, false};
        ValueGuard<bool> rec_exit{rec_should_exit, false};
        ValueGuard<bool> autotrack_exit{autotrack_should_exit, false};
        TempDir dir{"cli"};

        CommandEnv() { satdump::satdump_cfg.main_cfg["satdump_directories"]["default_input_directory"]["value"] = dir.str(); }
    };

    // Sample k of the test baseband is (k, -k)
    inline std::string writeRampBaseband(const TempDir &dir, int samples, const std::string &name = "input.cf32")
    {
        std::vector<complex_t> data(samples);
        for (int i = 0; i < samples; i++)
            data[i] = complex_t((float)i, -(float)i);
        std::string path = dir.file(name);
        test_helpers::writeBytes(path, data);
        return path;
    }

#ifndef _WIN32
    ///////////////////////////////////////////////////////////
    // POSIX only: stderr capture and a minimal HTTP client
    ///////////////////////////////////////////////////////////

    // Redirects the process stderr to a file until stop() or destruction. main_old() creates
    // its own logger writing there, which LogCapture cannot intercept.
    class StderrCapture
    {
    private:
        test_helpers::TempFile file{"stderr.txt"};
        int saved_fd = -1;

    public:
        StderrCapture()
        {
            fflush(stderr);
            saved_fd = dup(STDERR_FILENO);
            int fd = open(file.str().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fd >= 0)
            {
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
        }

        ~StderrCapture() { stop(); }

        StderrCapture(const StderrCapture &) = delete;
        StderrCapture &operator=(const StderrCapture &) = delete;

        // Restores stderr and returns what was written to it
        std::string stop()
        {
            if (saved_fd >= 0)
            {
                fflush(stderr);
                dup2(saved_fd, STDERR_FILENO);
                close(saved_fd);
                saved_fd = -1;
            }
            return test_helpers::readFile(file.path);
        }
    };

    struct HttpResponse
    {
        bool ok = false;  // A complete response was received
        int status = 0;   // HTTP status code
        std::string head; // Status line and headers
        std::string body;

        bool hasHeader(const std::string &line) const { return head.find(line) != std::string::npos; }
    };

    // One HTTP/1.0 request to 127.0.0.1:port. Every blocking step is bounded, so a server that
    // does not answer gives ok == false instead of hanging the test.
    inline HttpResponse httpRequest(int port, const std::string &path, const std::string &method = "GET")
    {
        HttpResponse res;

        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            return res;

        timeval tv{10, 0};
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);
        if (connect(fd, (sockaddr *)&addr, sizeof(addr)) != 0)
        {
            close(fd);
            return res;
        }

        std::string req = method + " " + path + " HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n";
        if (send(fd, req.data(), req.size(), MSG_NOSIGNAL) != (ssize_t)req.size())
        {
            close(fd);
            return res;
        }

        // HTTP/1.0 without keep-alive: the server closes the connection after the response
        std::string raw;
        char buf[4096];
        while (true)
        {
            pollfd p{fd, POLLIN, 0};
            if (poll(&p, 1, 10000) <= 0)
                break;
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if (n <= 0)
            {
                res.ok = n == 0;
                break;
            }
            raw.append(buf, n);
        }
        close(fd);

        size_t split = raw.find("\r\n\r\n");
        if (split == std::string::npos || raw.size() < 12)
        {
            res.ok = false;
            return res;
        }
        res.head = raw.substr(0, split + 2);
        res.body = raw.substr(split + 4);
        res.status = atoi(raw.substr(9, 3).c_str());
        return res;
    }

    // True once something accepts connections on 127.0.0.1:port
    inline bool portIsOpen(int port)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            return false;
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);
        bool open = connect(fd, (sockaddr *)&addr, sizeof(addr)) == 0;
        close(fd);
        return open;
    }

    inline std::string loopback(int port) { return "127.0.0.1:" + std::to_string(port); }
#endif
} // namespace cli_test
