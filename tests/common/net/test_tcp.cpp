#include "common/net/tcp.h"
#include <catch2/catch_test_macros.hpp>
#include <netinet/in.h>
#include <string>
#include <sys/time.h>

namespace
{
    // Minimal loopback listener on an ephemeral port
    struct Listener
    {
        int fd = -1;
        int port = 0;

        Listener()
        {
            fd = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = 0;
            bind(fd, (sockaddr *)&addr, sizeof(addr));
            listen(fd, 4);
            socklen_t len = sizeof(addr);
            getsockname(fd, (sockaddr *)&addr, &len);
            port = ntohs(addr.sin_port);
        }

        // Accept with a receive timeout so a broken test cannot hang the suite
        int accept_client()
        {
            int c = accept(fd, nullptr, nullptr);
            timeval tv = {2, 0};
            setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            return c;
        }

        ~Listener() { close(fd); }
    };
} // namespace

TEST_CASE("net::TCPClient exchanges data with a loopback server", "[common][net][tcp]")
{
    Listener listener;
    REQUIRE(listener.port != 0);

    char address[] = "127.0.0.1";
    net::TCPClient client(address, listener.port);
    int conn = listener.accept_client();
    REQUIRE(conn >= 0);

    std::string msg = "ping";
    CHECK(client.sends((uint8_t *)msg.data(), msg.size()) == 4);

    char buf[16] = {};
    int got = 0;
    while (got < 4)
    {
        int r = recv(conn, buf + got, sizeof(buf) - got, 0);
        if (r <= 0)
            break;
        got += r;
    }
    REQUIRE(got == 4);
    CHECK(std::string(buf, 4) == "ping");

    std::string reply = "pong!";
    REQUIRE(send(conn, reply.data(), reply.size(), 0) == 5);
    uint8_t rbuf[16] = {};
    int rgot = 0;
    while (rgot < 5)
    {
        int r = client.recvs(rbuf + rgot, sizeof(rbuf) - rgot);
        if (r <= 0)
            break;
        rgot += r;
    }
    REQUIRE(rgot == 5);
    CHECK(std::string((char *)rbuf, 5) == reply);

    // Peer close reads as 0 bytes
    close(conn);
    CHECK(client.recvs(rbuf, sizeof(rbuf)) == 0);
}

TEST_CASE("net::TCPClient throws when nothing listens", "[common][net][tcp]")
{
    int port;
    {
        Listener l; // Grab a free port, then close it
        port = l.port;
    }
    char address[] = "127.0.0.1";
    CHECK_THROWS_AS(net::TCPClient(address, port), std::runtime_error);
}
