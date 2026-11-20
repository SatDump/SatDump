#pragma once

/*
Loopback socket helpers for the network tests, shared by the whole suite.
Everything is inline and lives in the test_helpers namespace, as every test
file is linked into the same executable. POSIX sockets only, so the whole
header is empty on Windows: wrap its users in #ifndef _WIN32 as well.
*/

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace test_helpers
{
    // A 127.0.0.1 port with nothing listening on it: bound to an ephemeral port, then closed
    inline int closedPort()
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        socklen_t len = sizeof(addr);
        bind(fd, (sockaddr *)&addr, sizeof(addr));
        getsockname(fd, (sockaddr *)&addr, &len);
        close(fd);
        return ntohs(addr.sin_port);
    }
} // namespace test_helpers
#endif
