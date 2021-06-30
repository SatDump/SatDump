#include "rtl_tcp_client.h"

#ifndef DISABLE_SDR_RTLTCP
#include "logger.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

RTLTCPClient::RTLTCPClient()
{
    is_connected = false;
}

bool RTLTCPClient::connectClient(std::string address, int port)
{
    logger->info("Connecting to RTL-TCP " + address + ":" + std::to_string(port));

#ifdef _WIN32
    WSADATA wsaData;
    socket_fd = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    hints;

    // Initialize Winsock
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0)
    {
        logger->error("Could not connect open socket!");
        return false;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    ret = getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (ret != 0)
    {
        logger->error("Could not connect get RTL-TCP Server address info!");
        WSACleanup();
        return false;
    }

    // Connect to server.
    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    ret = connect(socket_fd, result->ai_addr, (int)result->ai_addrlen);
    if (ret == SOCKET_ERROR)
    {
        logger->error("Could not connect to RTL-TCP Server!");
        closesocket(socket_fd);
        return false;
    }

    freeaddrinfo(result);
#else
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0)
    {
        logger->error("Could not connect open socket!");
        return false;
    }

    sockaddr_in server_address;
    hostent *server = gethostbyname(address.c_str());

    memset(&server_address, 0, sizeof(sockaddr_in));
    server_address.sin_family = AF_INET;
    memcpy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(uint16_t(port));

    int ret = connect(socket_fd, (sockaddr *)&server_address, sizeof(server_address));

    if (ret < 0)
    {
        logger->error("Could not connect to RTL-TCP Server!");
        return false;
    }
#endif

    logger->info("Success connecting to RTL-TCP!");
    is_connected = true;

    return true;
}

void RTLTCPClient::disconnect()
{
    if (is_connected)
    {
#ifdef _WIN32
        closesocket(socket_fd);
        WSACleanup();
#else
        close(socket_fd);
#endif
        is_connected = false;
    }
}

void RTLTCPClient::receiveSamples(uint8_t *buffer, int size)
{
    if (is_connected)
    {
#ifdef _WIN32
        recv(socket_fd, (char *)buffer, size, 0);
#else
        read(socket_fd, buffer, size);
#endif
    }
}

struct rtl_tcp_command
{
    unsigned char cmd;
    unsigned int prm;
};

void RTLTCPClient::sendCmd(uint8_t cmd, uint32_t prm)
{
    if (is_connected)
    {
        rtl_tcp_command command = {cmd, htonl(prm)};

#ifdef _WIN32
        send(socket_fd, (char *)&command, sizeof(rtl_tcp_command), 0);
#else
        write(socket_fd, (char *)&command, sizeof(rtl_tcp_command));
#endif
    }
}

void RTLTCPClient::setSamplerate(double samplerate)
{
    sendCmd(2, samplerate);
}

void RTLTCPClient::setFrequency(double frequency)
{
    sendCmd(1, frequency);
}

void RTLTCPClient::setGainMode(bool mode)
{
    sendCmd(3, mode);
}

void RTLTCPClient::setGain(int gain)
{
    sendCmd(4, gain);
}

void RTLTCPClient::setAGCMode(bool enable)
{
    sendCmd(8, enable);
}
#endif