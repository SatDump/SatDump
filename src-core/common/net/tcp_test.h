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
    }

    ~TCPServer()
    {
        thread_should_run = false;
        if (rx_thread.joinable())
            rx_thread.join();
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
        uint8_t *buffer = new uint8_t[100000];
        while (thread_should_run)
        {
            if (clientsockfd != -1)
            {
                int lpkt_size = sread(buffer, 2);
                if (lpkt_size == -1)
                {
                    clientsockfd = -1;
                    continue;
                }

                int current_pkt_size = lpkt_size;
                int expected_pkt_size = (buffer[0] << 8 | buffer[1]) + 2;
                while (current_pkt_size < expected_pkt_size)
                {
                    int ret = sread(buffer + current_pkt_size, expected_pkt_size - current_pkt_size);
                    if (ret == 0)
                        break;
                    current_pkt_size += ret;
                }

                callback_func(buffer + 2, expected_pkt_size - 2);
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
        std::vector<uint8_t> buff2;
        buff2.resize(len + 2);
        buff2[0] = len >> 8;
        buff2[1] = len & 0xFF;
        memcpy(&buff2[2], buff, len);
        write(clientsockfd, buff2.data(), buff2.size());
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
    }

    ~TCPClient()
    {
        thread_should_run = false;
        if (rx_thread.joinable())
            rx_thread.join();
        close(clientsockfd);
    }

public:
    void rx_thread_func()
    {
        uint8_t *buffer = new uint8_t[100000];
        int current_pkt_size = 0;
        int current_pkt_size_exp = -1;
        while (thread_should_run)
        {
            if (clientsockfd != -1)
            {
                int lpkt_size = sread(buffer, 2);
                if (lpkt_size == -1)
                    continue;

                if (current_pkt_size_exp == -1)
                {
                    current_pkt_size_exp = (buffer[0] << 8 | buffer[1]) + 2;
                    current_pkt_size = 0;
                }

                int current_pkt_size = lpkt_size;
                int expected_pkt_size = (buffer[0] << 8 | buffer[1]) + 2;
                while (current_pkt_size < expected_pkt_size)
                {
                    int ret = sread(buffer + current_pkt_size, expected_pkt_size - current_pkt_size);
                    if (ret == 0)
                        break;
                    current_pkt_size += ret;
                }

                callback_func(buffer + 2, expected_pkt_size - 2);

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
        std::vector<uint8_t> buff2;
        buff2.resize(len + 2);
        buff2[0] = len >> 8;
        buff2[1] = len & 0xFF;
        memcpy(&buff2[2], buff, len);
        write(clientsockfd, buff2.data(), buff2.size());
        //  logger->critical("HEADESENTR %d %d", len, buff2.size());
        write_mtx.unlock();
    }
};
