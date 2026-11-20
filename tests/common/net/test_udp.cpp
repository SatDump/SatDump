#include "common/net/udp.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <netinet/in.h>
#include <string>

namespace
{
    // Ask the kernel for a free UDP port by binding to port 0
    int findFreeUdpPort()
    {
        int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = 0;
        bind(s, (sockaddr *)&addr, sizeof(addr));
        socklen_t len = sizeof(addr);
        getsockname(s, (sockaddr *)&addr, &len);
        close(s);
        return ntohs(addr.sin_port);
    }

    // Opens a server on a free port, retrying in case of a race with another process
    std::unique_ptr<net::UDPServer> openServer(int &port)
    {
        for (int attempt = 0; attempt < 10; attempt++)
        {
            port = findFreeUdpPort();
            try
            {
                return std::make_unique<net::UDPServer>(port);
            }
            catch (std::exception &)
            {
            }
        }
        return nullptr;
    }

    // Receive with the server's 10ms timeout, retrying for up to ~2 seconds
    int recvWithRetry(net::UDPServer &server, uint8_t *buf, int len)
    {
        for (int i = 0; i < 200; i++)
        {
            try
            {
                return server.recv(buf, len);
            }
            catch (std::exception &)
            {
            }
        }
        return -1;
    }
} // namespace

TEST_CASE("net::UDPClient sends datagrams to net::UDPServer over loopback", "[common][net][udp]")
{
    int port = 0;
    auto server = openServer(port);
    REQUIRE(server != nullptr);
    server->enableTimeout();

    char address[] = "127.0.0.1";
    net::UDPClient client(address, port);

    std::string msg = "hello satdump";
    CHECK(client.send((uint8_t *)msg.data(), msg.size()) == (int)msg.size());

    uint8_t buf[64] = {};
    int n = recvWithRetry(*server, buf, sizeof(buf));
    REQUIRE(n == (int)msg.size());
    CHECK(std::string((char *)buf, n) == msg);

    // The server replies to the address it last received from
    std::string reply = "pong";
    CHECK(server->send((uint8_t *)reply.data(), reply.size()) == (int)reply.size());
    uint8_t rbuf[16] = {};
    int rn = client.recv(rbuf, sizeof(rbuf));
    REQUIRE(rn == 4);
    CHECK(std::string((char *)rbuf, rn) == reply);
}

TEST_CASE("net::UDPServer preserves datagram boundaries", "[common][net][udp]")
{
    int port = 0;
    auto server = openServer(port);
    REQUIRE(server != nullptr);
    server->enableTimeout();

    char address[] = "127.0.0.1";
    net::UDPClient client(address, port);

    uint8_t a[3] = {1, 2, 3};
    uint8_t b[5] = {4, 5, 6, 7, 8};
    client.send(a, 3);
    client.send(b, 5);

    uint8_t buf[64];
    CHECK(recvWithRetry(*server, buf, sizeof(buf)) == 3);
    CHECK(buf[0] == 1);
    CHECK(recvWithRetry(*server, buf, sizeof(buf)) == 5);
    CHECK(buf[4] == 8);
}

TEST_CASE("net::UDPServer recv times out when enabled", "[common][net][udp]")
{
    int port = 0;
    auto server = openServer(port);
    REQUIRE(server != nullptr);
    server->enableTimeout();

    uint8_t buf[16];
    CHECK_THROWS_AS(server->recv(buf, sizeof(buf)), std::runtime_error);
}

TEST_CASE("net::UDPClient rejects an invalid address", "[common][net][udp]")
{
    char address[] = "not-an-ip-address";
    CHECK_THROWS_AS(net::UDPClient(address, 1234), std::runtime_error);
}

TEST_CASE("net::UDPServer fails to bind a port already in use", "[common][net][udp]")
{
    int port = 0;
    auto server = openServer(port);
    REQUIRE(server != nullptr);
    CHECK_THROWS_AS(net::UDPServer(port), std::runtime_error);
}
