#pragma once

#include "logger.h"

#include <sys/types.h>
#include <stdlib.h>
#include <cstring>
#include <thread>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <stdio.h>
#if defined(_WIN32)
#include <winsock2.h>
#include <WS2tcpip.h>
#define MSG_NOSIGNAL 0
#else
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

class TCPServer
{
private:
    const int d_port;
    int serversockfd = -1;

    struct sockaddr_in clientaddr;
    int clientsockfd = -1;

    bool thread_should_run = true;
    std::thread rx_thread;
    std::mutex write_mtx;

    uint8_t *buffer_tx;

#if defined(_WIN32)
    WSADATA wsa;
#endif

public:
    std::function<void(uint8_t *, int)> callback_func;
    std::function<void()> callback_func_on_lost_client;

public:
    TCPServer(int port) : d_port(port)
    {
#if defined(_WIN32)
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            throw std::runtime_error("Couldn't startup WSA socket!");
#endif

        serversockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serversockfd == -1)
            throw std::runtime_error("Socket creation failed");

#ifdef _WIN32
        int timeout = 60000;
        const char *timeout_ptr = (const char*) &timeout;
#else
        struct timeval timeout;
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;
        struct timeval *timeout_ptr = &timeout;
#endif
        int enable = 1;
        if (setsockopt(serversockfd, SOL_SOCKET, SO_SNDTIMEO, timeout_ptr, sizeof timeout) < 0)
            logger->trace("Problem setting send timeout on TCP socket; ignoring");

        if (setsockopt(serversockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof enable) < 0)
            logger->trace("Problem setting SO_REUSEADDR on TCP socket; ignoring");
#ifndef  _WIN32
        if (setsockopt(serversockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof enable) < 0)
            logger->trace("Problem setting SO_REUSEPORT on TCP socket; ignoring");
#endif

        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(d_port);

        if ((bind(serversockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
            throw std::runtime_error("Socket bind failed");
        else
            logger->trace("Socket successfully bound to TCP port %d", port);

        if ((listen(serversockfd, 5)) != 0)
            throw std::runtime_error("Listen failed");
        else
            logger->trace("Server listening");

        rx_thread = std::thread(&TCPServer::rx_thread_func, this);

        buffer_tx = new uint8_t[3000000];
    }

    ~TCPServer()
    {
        thread_should_run = false;
        if (rx_thread.joinable())
            rx_thread.join();

        delete[] buffer_tx;
    }

public:
    void wait_client()
    {
        try
        {
            logger->trace("Waiting for client!");
            struct sockaddr_in nclientaddr;
            int len = sizeof(nclientaddr);
            int nclientsockfd = accept(serversockfd, (struct sockaddr *)&nclientaddr, (socklen_t *)&len);
            if (nclientsockfd < 0)
                throw std::runtime_error("Server accept failed");
            else
                logger->trace("Server accepted the client");

            if (clientsockfd != -1)
            {
                uint8_t refused = 0x00;
                send(nclientsockfd, (char *)&refused, 1, 0);
                logger->trace("Still got client! Refusing request.");
#if defined(_WIN32)
                closesocket(nclientsockfd);
#else
                close(nclientsockfd);
#endif
            }
            else
            {
                uint8_t accepted = 0xFF;
                send(nclientsockfd, (char *)&accepted, 1, 0);
                clientaddr = nclientaddr;
                clientsockfd = nclientsockfd;
            }
        }
        catch (std::exception &e)
        {
            logger->error("Failed accepting client. %s", e.what());
        }
    }

public:
    void rx_thread_func()
    {
        uint8_t *buffer = new uint8_t[3000000];
        struct timeval timeout;
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        while (thread_should_run)
        {
            if (clientsockfd != -1)
            {
                fd_set socket_set;
                FD_ZERO(&socket_set);
                FD_SET(clientsockfd, &socket_set);
                if (select(clientsockfd + 1, &socket_set, nullptr, nullptr, &timeout) == 0)
                {
                    //Prevent timeout from hanging loop
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                int lpkt_size = sread(buffer, 4);
                if (lpkt_size <= 0)
                {
                    closeconn();
                    continue;
                }
                if (lpkt_size < 4)
                {
                    logger->error("Client sent a packet that was too small. Closing connection!");
                    closeconn();
                    continue;
                }

                int current_pkt_size = lpkt_size;
                int expected_pkt_size = uint32_t(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) + 4;
                if (expected_pkt_size > 3000000)
                {
                    logger->error("Reported packet size too large. Closing connection!");
                    closeconn();
                    continue;
                }

                while (current_pkt_size < expected_pkt_size)
                {
                    int ret = sread(buffer + current_pkt_size, expected_pkt_size - current_pkt_size);
                    if (ret <= 0)
                        break;
                    current_pkt_size += ret;
                }

                callback_func(buffer + 4, expected_pkt_size - 4);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        delete[] buffer;
    }

private:
    int sread(uint8_t *buff, int len)
    {
        // int ret = read(clientsockfd, buff, len);
        return recv(clientsockfd, (char *)buff, len, 0);
    }

public:
    int swrite(uint8_t *buff, int len)
    {
        int r = -1;
        write_mtx.lock();
        if (clientsockfd != -1)
        {
            buffer_tx[0] = (len >> 24) & 0xFF;
            buffer_tx[1] = (len >> 16) & 0xFF;
            buffer_tx[2] = (len >> 8) & 0xFF;
            buffer_tx[3] = len & 0xFF;
            memcpy(&buffer_tx[4], buff, len);
            // write(clientsockfd, buffer_tx, len + 4);
            r = send(clientsockfd, (char *)buffer_tx, len + 4, MSG_NOSIGNAL);
            // logger->critical("HEADESENTR %d %d", len, buff2.size());
        }
        write_mtx.unlock();
        return r;
    }

public:
    void closeconn()
    {
        logger->trace("Server lost client");
#if defined(_WIN32)
        closesocket(clientsockfd);
#else
        close(clientsockfd);
#endif
        clientsockfd = -1;
        callback_func_on_lost_client();
    }
};

class TCPClient
{
private:
    const int d_port;
    int clientsockfd = -1;

    bool thread_should_run = true;
    std::thread rx_thread;
    std::mutex write_mtx;

    uint8_t *buffer_tx;

#if defined(_WIN32)
    WSADATA wsa;
#endif

public:
    std::function<void(uint8_t *, int)> callback_func;
    bool readOne = false;

public:
    TCPClient(char *address, int port) : d_port(port)
    {
#if defined(_WIN32)
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            throw std::runtime_error("Couldn't startup WSA socket!");
#endif

        clientsockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientsockfd == -1)
            throw std::runtime_error("Socket creation failed");

#ifdef _WIN32
        int timeout = 10000;
        const char* timeout_ptr = (const char*)&timeout;
#else
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        struct timeval* timeout_ptr = &timeout;
#endif
        if (setsockopt(clientsockfd, SOL_SOCKET, SO_SNDTIMEO, timeout_ptr, sizeof timeout) < 0)
            logger->trace("Problem setting send timeout on TCP socket; ignoring");
        if (setsockopt(clientsockfd, SOL_SOCKET, SO_RCVTIMEO, timeout_ptr, sizeof timeout) < 0)
            logger->trace("Problem setting receive timeout on TCP socket; ignoring");

        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(address);
        servaddr.sin_port = htons(d_port);

        if (connect(clientsockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
            throw std::runtime_error("Connection with the server failed");

        uint8_t response = 0;
        if (recv(clientsockfd, (char *)&response, 1, 0) == -1)
            throw std::runtime_error("Receive from the server failed");

        if (response != 0xFF)
            throw std::runtime_error("Server refused client!");

        rx_thread = std::thread(&TCPClient::rx_thread_func, this);

        buffer_tx = new uint8_t[3000000];
    }

    ~TCPClient()
    {
        thread_should_run = false;
        if (rx_thread.joinable())
            rx_thread.join();
#if defined(_WIN32)
        closesocket(clientsockfd);
#else
        close(clientsockfd);
#endif

        delete[] buffer_tx;
    }

public:
    void rx_thread_func()
    {
        uint8_t *buffer = new uint8_t[3000000];
        int current_pkt_size_exp = -1;
        while (thread_should_run)
        {
            if (clientsockfd != -1)
            {
                int lpkt_size = sread(buffer, 4);
                if (lpkt_size <= 0)
                    continue;

                if (current_pkt_size_exp == -1)
                {
                    current_pkt_size_exp = uint32_t(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) + 4;
                }

                int current_pkt_size = lpkt_size;
                int expected_pkt_size = uint32_t(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) + 4;
                while (current_pkt_size < expected_pkt_size)
                {
                    int ret = sread(buffer + current_pkt_size, expected_pkt_size - current_pkt_size);
                    if (ret <= 0)
                        break;
                    current_pkt_size += ret;
                }

                callback_func(buffer + 4, expected_pkt_size - 4);

                if (readOne)
                    break;
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        delete[] buffer;
    }

private:
    int sread(uint8_t *buff, int len)
    {
        // return read(clientsockfd, buff, len);
        return recv(clientsockfd, (char *)buff, len, 0);
    }

public:
    int swrite(uint8_t *buff, int len)
    {
        write_mtx.lock();
        buffer_tx[0] = (len >> 24) & 0xFF;
        buffer_tx[1] = (len >> 16) & 0xFF;
        buffer_tx[2] = (len >> 8) & 0xFF;
        buffer_tx[3] = len & 0xFF;
        memcpy(&buffer_tx[4], buff, len);
        // write(clientsockfd, buffer_tx, len + 4);
        int ret = send(clientsockfd, (char *)buffer_tx, len + 4, MSG_NOSIGNAL);
        //  logger->critical("HEADESENTR %d %d", len, buff2.size());
        write_mtx.unlock();
        return ret;
    }

public:
    void closeconn()
    {
        readOne = true;
    }
};
