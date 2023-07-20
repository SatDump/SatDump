#pragma once

#include <string.h>
#include <stdexcept>

#if defined(_WIN32)
#include <stdio.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace net
{
    class TCPClient
    {
    private:
        sockaddr_in sock_addr;
#if defined(_WIN32)
        WSADATA wsa;
#endif
        int sock = 0;

    public:
        TCPClient(char *address, int port)
        {
#if defined(_WIN32)
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
                throw std::runtime_error("Couldn't startup WSA socket!");
#endif

            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
                throw std::runtime_error("Couldn't open TCP socket!");

            memset((char *)&sock_addr, 0, sizeof(sock_addr));
            sock_addr.sin_family = AF_INET;
            sock_addr.sin_addr.s_addr = inet_addr(address);
            sock_addr.sin_port = htons(port);

            if (connect(sock, (const sockaddr *)&sock_addr, sizeof(sock_addr)) != 0)
                throw std::runtime_error("Couldn't connect to TCP socket!");
        }

        ~TCPClient()
        {
#if defined(_WIN32)
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
        }

        int sends(uint8_t *data, int len)
        {
            int r = send(sock, (char *)data, len, 0);
            if (r == -1)
                throw std::runtime_error("Error sending to TCP socket!");
            return r;
        }

        int recvs(uint8_t *data, int len)
        {
            int r = recv(sock, (char *)data, len, 0);
            if (r == -1)
                throw std::runtime_error("Error receiving from TCP socket!");
            return r;
        }
    };
}