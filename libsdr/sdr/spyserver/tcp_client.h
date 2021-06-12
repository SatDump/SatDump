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

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include <atomic>
#include <chrono>
#include <thread>
#include <cstdint>
#include <stdint.h>
#include <memory.h>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#   include <winsock2.h>
#   include <Ws2tcpip.h>
#else
#   include <sys/socket.h>
#   include <netinet/in.h>
#endif


#if defined(__GNUC__) || defined(__MINGW32__)
#include <unistd.h>
#endif


class tcp_client {
private:
    int port;
    #ifdef _WIN32
    static std::atomic_bool initialized;
    static std::atomic_uint sockCount;
    void socket_initialize();
    #endif
protected:
    struct sockaddr_in socketAddr;
    int s;
public:
    tcp_client() {}
    tcp_client(std::string addr, int port);
    ~tcp_client();

    void connect_conn();
    void close_conn();

    void receive_data(char *data, int length);
    void send_data(char *data, int length);
    uint64_t available_data();

    inline void wait_for_data(uint64_t bytes, uint32_t timeout) {
        uint32_t checkTime = (int) time(NULL);
        while (available_data() < bytes) {
            if (((int) time(NULL)) - checkTime > timeout) {
                return;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
};

#endif /* TCPCLIENT_H_ */
