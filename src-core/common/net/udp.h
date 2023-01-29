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
    class UDPClient
    {
    private:
        sockaddr_in sock_addr;
#if defined(_WIN32)
        WSADATA wsa;
#endif
        int sock = 0;

    public:
        UDPClient(char *address, int port)
        {
#if defined(_WIN32)
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
                throw std::runtime_error("Couldn't startup WSA socket!");
#endif

            if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
                throw std::runtime_error("Couldn't open UDP socket!");

            memset((char *)&sock_addr, 0, sizeof(sock_addr));
            sock_addr.sin_family = AF_INET;
            sock_addr.sin_port = htons(port);

#if defined(_WIN32)
            sock_addr.sin_addr.S_un.S_addr = inet_addr(address);
#else
            if (inet_aton(address, &sock_addr.sin_addr) == 0)
                throw std::runtime_error("Couldn't connect to UDP socket!");
#endif
        }

        ~UDPClient()
        {
#if defined(_WIN32)
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
        }

        int send(uint8_t *data, int len)
        {
            int slen = sizeof(sockaddr);
            int r = sendto(sock, (char *)data, len, 0, (sockaddr *)&sock_addr, slen);
            if (r == -1)
                throw std::runtime_error("Error sending to UDP socket!");
            return r;
        }

        int recv(uint8_t *data, int len)
        {
#if defined(_WIN32)
            int slen = sizeof(sockaddr);
#else
            socklen_t slen = sizeof(sockaddr);
#endif
            int r = recvfrom(sock, (char *)data, len, 0, (struct sockaddr *)&sock_addr, &slen);
            if (r == -1)
                throw std::runtime_error("Error receiving from UDP socket!");
            return r;
        }
    };
}