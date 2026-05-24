#pragma once

#include <stdexcept>
#include <string.h>

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
        SOCKET sock = INVALID_SOCKET;
#else
        int sock = -1;
#endif

    public:
        UDPClient(const UDPClient &) = delete;
        UDPClient &operator=(const UDPClient &) = delete;

        UDPClient(char *address, int port)
        {
#if defined(_WIN32)
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
                throw std::runtime_error("Couldn't startup WSA socket!");
#endif

#if defined(_WIN32)
            if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
#else
            if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
#endif
                throw std::runtime_error("Couldn't open UDP socket!");

            memset((char *)&sock_addr, 0, sizeof(sock_addr));
            sock_addr.sin_family = AF_INET;
            sock_addr.sin_port = htons(port);

#if defined(_WIN32)
            sock_addr.sin_addr.S_un.S_addr = inet_addr(address);
            if (sock_addr.sin_addr.S_un.S_addr == INADDR_NONE)
                throw std::runtime_error("Couldn't connect to UDP socket!");
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
#if defined(_WIN32)
            if (r == SOCKET_ERROR)
#else
            if (r < 0)
#endif

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
#if defined(_WIN32)
            if (r == SOCKET_ERROR)
#else
            if (r < 0)
#endif

                throw std::runtime_error("Error receiving from UDP socket!");
            return r;
        }
    };

    class UDPServer
    {
    private:
        sockaddr_in sock_addr;
#if defined(_WIN32)
        WSADATA wsa;
        SOCKET sock = INVALID_SOCKET;
#else
        int sock = -1;
#endif

    public:
        UDPServer(const UDPServer &) = delete;
        UDPServer &operator=(const UDPServer &) = delete;

        UDPServer(int port)
        {
#if defined(_WIN32)
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
                throw std::runtime_error("Couldn't startup WSA socket!");
#endif

#if defined(_WIN32)
            if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
#else
            if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
#endif
                throw std::runtime_error("Couldn't open UDP socket!");

            memset((char *)&sock_addr, 0, sizeof(sock_addr));
            sock_addr.sin_family = AF_INET;
            sock_addr.sin_addr.s_addr = INADDR_ANY;
            sock_addr.sin_port = htons(port);

#if defined(_WIN32)
            sock_addr.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            if (bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0)
                throw std::runtime_error("Couldn't connect to UDP socket!");
            int ttrue = 1;
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&ttrue, sizeof(int));
        }

        ~UDPServer()
        {
#if defined(_WIN32)
            closesocket(sock);
            WSACleanup();
#else
            shutdown(sock, SHUT_RDWR);
            close(sock);
#endif
        }

        void enableTimeout()
        {
#if defined(_WIN32)
            DWORD read_timeout = 10;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&read_timeout, sizeof read_timeout);
#else
            struct timeval read_timeout;
            read_timeout.tv_sec = 0;
            read_timeout.tv_usec = 1e4;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&read_timeout, sizeof read_timeout);
#endif
        }

        int send(uint8_t *data, int len)
        {
            int slen = sizeof(sockaddr);
            int r = sendto(sock, (char *)data, len, 0, (sockaddr *)&sock_addr, slen);
#if defined(_WIN32)
            if (r == SOCKET_ERROR)
#else
            if (r < 0)
#endif
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
#if defined(_WIN32)
            if (r == SOCKET_ERROR)
#else
            if (r < 0)
#endif
            {
#if defined(_WIN32)
                if (WSAGetLastError() != WSAEINTR)
#endif
                    throw std::runtime_error("Error receiving from UDP socket!");
            }
            return r;
        }
    };
} // namespace net
