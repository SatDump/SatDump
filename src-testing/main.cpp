/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"

#include <sys/types.h>
#include <stdexcept>
#include <cstring>
#if defined(_WIN32)
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

    int the_port = 8877;

#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        throw std::runtime_error("Couldn't startup WSA socket!");
#endif

    struct sockaddr_in recv_addr;
    int fd = -1;

    if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        throw std::runtime_error("Error creating socket!");

    int val_true = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&val_true, sizeof(val_true)) < 0)
        throw std::runtime_error("Error setting socket option!");

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(the_port);
    recv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0)
        throw std::runtime_error("Error binding socket!");

    uint8_t buffer_rx[65536];

    while (true)
    {
        struct sockaddr_in response_addr;
        socklen_t response_addr_len = sizeof(response_addr);
        int nrecv = recvfrom(fd, (char *)buffer_rx, 65536, 0, (struct sockaddr *)&response_addr, &response_addr_len);
        if (nrecv < 0)
            throw std::runtime_error("Error on recvfrom!");

        std::string command = (char *)buffer_rx;

        std::string name = command.substr(20, 11);
        std::string freq = command.substr(69, 11);

        logger->trace(name);
        logger->info("%f", std::stof(freq));
    }

#if defined(_WIN32)
    closesocket(fd);
    WSACleanup();
#else
    close(fd);
#endif
}