#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <functional>

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

public:
    std::function<void(uint8_t *, int)> callback_func;

public:
    TCPServer(int port)
        : d_port(port)
    {
        serversockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (serversockfd == -1)
            throw std::runtime_error("Socket creation failed");

        struct sockaddr_in servaddr;
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(d_port);

        if ((bind(serversockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
            throw std::runtime_error("Socket bind failed");
        else
            logger->trace("Socket successfully binded");

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

private:
    void wait_client()
    {
        logger->trace("Waiting for client!");
        int len = sizeof(clientaddr);
        clientsockfd = accept(serversockfd, (struct sockaddr *)&clientaddr, (socklen_t *)&len);
        if (clientsockfd < 0)
            throw std::runtime_error("Server accept failed");
        else
            logger->trace("server accept the client");
    }

public:
    void rx_thread_func()
    {
        uint8_t *buffer = new uint8_t[3000000];
        while (thread_should_run)
        {
            if (clientsockfd != -1)
            {
                int lpkt_size = sread(buffer, 4);
                if (lpkt_size == -1)
                {
                    clientsockfd = -1;
                    continue;
                }

                int current_pkt_size = lpkt_size;
                int expected_pkt_size = uint32_t(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) + 4;
                while (current_pkt_size < expected_pkt_size)
                {
                    int ret = sread(buffer + current_pkt_size, expected_pkt_size - current_pkt_size);
                    if (ret == 0)
                        break;
                    current_pkt_size += ret;
                }

                callback_func(buffer + 4, expected_pkt_size - 4);
            }
            else
            {
                wait_client();
            }
        }
        delete[] buffer;
    }

private:
    int sread(uint8_t *buff, int len)
    {
        int ret = read(clientsockfd, buff, len);
        if (ret == 0)
        {
            logger->trace("Server lost client");
            close(clientsockfd);
            clientsockfd = -1;
            ret = -1;
        }
        return ret;
    }

public:
    void swrite(uint8_t *buff, int len)
    {
        write_mtx.lock();
        buffer_tx[0] = (len >> 24) & 0xFF;
        buffer_tx[1] = (len >> 16) & 0xFF;
        buffer_tx[2] = (len >> 8) & 0xFF;
        buffer_tx[3] = len & 0xFF;
        memcpy(&buffer_tx[4], buff, len);
        write(clientsockfd, buffer_tx, len + 4);
        // logger->critical("HEADESENTR %d %d", len, buff2.size());
        write_mtx.unlock();
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

public:
    std::function<void(uint8_t *, int)> callback_func;
    bool readOne = false;

public:
    TCPClient(char *address, int port)
        : d_port(port)
    {
        clientsockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (clientsockfd == -1)
            throw std::runtime_error("Socket creation failed");

        struct sockaddr_in servaddr;
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(address);
        servaddr.sin_port = htons(d_port);

        if (connect(clientsockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
            throw std::runtime_error("Connection with the server failed");
        // else
        //     logger->trace("Connected to the server");

        rx_thread = std::thread(&TCPClient::rx_thread_func, this);

        buffer_tx = new uint8_t[3000000];
    }

    ~TCPClient()
    {
        thread_should_run = false;
        if (rx_thread.joinable())
            rx_thread.join();
        close(clientsockfd);

        delete[] buffer_tx;
    }

public:
    void rx_thread_func()
    {
        uint8_t *buffer = new uint8_t[3000000];
        int current_pkt_size = 0;
        int current_pkt_size_exp = -1;
        while (thread_should_run)
        {
            if (clientsockfd != -1)
            {
                int lpkt_size = sread(buffer, 4);
                if (lpkt_size == -1)
                    continue;

                if (current_pkt_size_exp == -1)
                {
                    current_pkt_size_exp = uint32_t(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) + 4;
                    current_pkt_size = 0;
                }

                int current_pkt_size = lpkt_size;
                int expected_pkt_size = uint32_t(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) + 4;
                while (current_pkt_size < expected_pkt_size)
                {
                    int ret = sread(buffer + current_pkt_size, expected_pkt_size - current_pkt_size);
                    if (ret == 0)
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
        return read(clientsockfd, buff, len);
    }

public:
    void swrite(uint8_t *buff, int len)
    {
        write_mtx.lock();
        buffer_tx[0] = (len >> 24) & 0xFF;
        buffer_tx[1] = (len >> 16) & 0xFF;
        buffer_tx[2] = (len >> 8) & 0xFF;
        buffer_tx[3] = len & 0xFF;
        memcpy(&buffer_tx[4], buff, len);
        write(clientsockfd, buffer_tx, len + 4);
        //  logger->critical("HEADESENTR %d %d", len, buff2.size());
        write_mtx.unlock();
    }
};
