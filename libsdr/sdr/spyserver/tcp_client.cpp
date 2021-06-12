/* -*- c++ -*- */
/*
 * Copyright 2018 Lucas Teske <lucas@teske.com.br>
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "tcp_client.h"

#include <cstdio>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <atomic>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#ifndef MSG_WAITALL
#define MSG_WAITALL (1 << 3)
#endif
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>
#define ioctlsocket ioctl
#endif
#if defined(_WIN32) || defined(__APPLE__)
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#endif

#ifdef _WIN32
std::atomic_bool tcp_client::initialized(false);
std::atomic_uint tcp_client::sockCount(0);

void tcp_client::socket_initialize()
{
    if (!initialized)
    {
        initialized = true;
        sockCount = 1;
        WSADATA wsa_data;
        WSAStartup(MAKEWORD(1, 1), &wsa_data);
    }
    else
    {
        sockCount++;
    }
}
#endif

tcp_client::tcp_client(std::string addr, int port)
{
    this->port = port;
#ifdef _WIN32
    socket_initialize();
#endif
    hostent *record = gethostbyname(addr.c_str());
    if (record == NULL)
    {
        throw std::runtime_error(std::string(__FUNCTION__) + " " +
                                 "Cannot resolve: " + addr);
    }
    in_addr *address = (in_addr *)record->h_addr;
    memset(&socketAddr, 0x00, sizeof(sockaddr_in));
    socketAddr.sin_addr = *address;
}

void tcp_client::connect_conn()
{
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        throw std::runtime_error("Socket Error Code " + std::to_string(errno));
    }

    socketAddr.sin_family = AF_INET;
    socketAddr.sin_port = htons(port);
    int x = connect(s, (struct sockaddr *)&socketAddr, sizeof(socketAddr));
    if (x < 0)
    {
        throw std::runtime_error("Socket Error Code " + std::to_string(errno));
    }
}

void tcp_client::close_conn()
{
    if (s > 0)
    {
        int status = 0;
#ifdef _WIN32
        status = shutdown(s, SD_BOTH);
        if (status == 0)
        {
            status = closesocket(s);
        }
#else
        status = shutdown(s, 2);
        if (status == 0)
        {
            status = close(s);
        }
#endif
    }
}

tcp_client::~tcp_client()
{
#ifdef _WIN32
    sockCount--;
    if (!sockCount)
    {
        WSACleanup();
    }
#endif
}

void tcp_client::receive_data(char *data, int length)
{
    long n = recv(s, data, length, MSG_WAITALL);
    if (n == 0)
    {
        throw std::runtime_error("Client Disconnected");
    }
    else if (n != length)
    {
        throw std::runtime_error("Socket Error Code " + std::to_string(errno));
    }
}

void tcp_client::send_data(char *data, int length)
{
    int n = send(s, data, length, MSG_NOSIGNAL);
    if (n == 0)
    {
        throw std::runtime_error("Client Disconnected");
    }
    else if (n != length)
    {
        throw std::runtime_error("Socket Error Code " + std::to_string(errno));
    }
}

uint64_t tcp_client::available_data()
{
    if (s < 0)
    {
        return 0;
    }

    unsigned long bytesAvailable = 0;
    int ret = ioctlsocket(s, FIONREAD, &bytesAvailable);

    switch (ret)
    {
    case EINVAL:
    case EFAULT:
    case ENOTTY:
        throw std::runtime_error("Socket Error Code " + std::to_string(ret));
        break;
    case EBADF:
        throw std::runtime_error("Client Disconnected");
        break;
    }

    return bytesAvailable;
}
