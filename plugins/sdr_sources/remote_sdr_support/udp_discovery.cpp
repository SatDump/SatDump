#include "udp_discovery.h"

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

#include "logger.h"

namespace service_discovery
{
    // Lil helper function
    void cleanup_socket(int fd)
    {
#if defined(_WIN32)
        closesocket(fd);
        WSACleanup();
#else
        close(fd);
#endif
    }

    void sendUdpBroadcast(int port, uint8_t *data, int len)
    {
#if defined(_WIN32)
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            throw std::runtime_error("Couldn't startup WSA socket!");
#endif

        struct sockaddr_in send_addr;
        int fd = -1;

        if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            throw std::runtime_error("Error creating socket!");

        int val_true = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char *)&val_true, sizeof(val_true)) < 0)
            throw std::runtime_error("Error setting socket option!");

        memset(&send_addr, 0, sizeof(send_addr));
        send_addr.sin_family = AF_INET;
        send_addr.sin_port = htons(port);
        send_addr.sin_addr.s_addr = INADDR_BROADCAST;

        if (sendto(fd, (const char *)data, len, 0, (struct sockaddr *)&send_addr, sizeof(sockaddr)) < 0)
            throw std::runtime_error(std::strerror(errno));

        cleanup_socket(fd);
    }

    void sendUdpPacket(const char *address, int port, uint8_t *data, int len)
    {
#if defined(_WIN32)
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            throw std::runtime_error("Couldn't startup WSA socket!");
#endif

        struct sockaddr_in send_addr;
        int fd = -1;

        if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            throw std::runtime_error("Error creating socket!");

        memset(&send_addr, 0, sizeof(send_addr));
        send_addr.sin_family = AF_INET;
        send_addr.sin_port = htons(port);
        // send_addr.sin_addr.s_addr = INADDR_BROADCAST;
#if defined(_WIN32)
        send_addr.sin_addr.S_un.S_addr = inet_addr(address);
#else
        inet_aton(address, &send_addr.sin_addr);
#endif

        int ret = sendto(fd, (char *)data, len, 0, (struct sockaddr *)&send_addr, sizeof(sockaddr));
        if (ret < 0)
            throw std::runtime_error("Error on send!");

        cleanup_socket(fd);
    }

    void UDPDiscoveryServerRunner::discovery_thread()
    {
#if defined(_WIN32)
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        {
            logger->error("Couldn't startup WSA socket for UDP discovery requests!");
            return;
        }
#endif

        struct sockaddr_in recv_addr;
        int fd = -1;

        if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
            logger->error("Error creating UDP discovery socket!");
            cleanup_socket(fd);
            return;
        }

        int val_true = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val_true, sizeof(val_true)) < 0)
        {
            logger->error("Error setting UDP discovery socket option!");
            cleanup_socket(fd);
            return;
        }

        memset(&recv_addr, 0, sizeof(recv_addr));
        recv_addr.sin_family = AF_INET;
        recv_addr.sin_port = htons(cfg.req_port);
        recv_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(fd, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) < 0)
        {
            logger->error("Error binding UDP discovery socket!");
            cleanup_socket(fd);
            return;
        }

        uint8_t *buffer_rx = new uint8_t[65536];
        while (should_run)
        {
            struct sockaddr_in response_addr;
            socklen_t response_addr_len = sizeof(response_addr);
            int nrecv = recvfrom(fd, (char *)buffer_rx, 65536, 0, (struct sockaddr *)&response_addr, &response_addr_len);
            if (nrecv < 0)
            {
                logger->error("Error on UDP discovery recvfrom!");
                break;
            }

            bool is_valid = true;
            if (nrecv == (int)cfg.req_pkt.size())
            {
                for (size_t i = 0; i < cfg.req_pkt.size(); i++)
                    if (buffer_rx[i] != cfg.req_pkt[i])
                        is_valid = false;
            }
            else
            {
                is_valid = false;
            }

            if (is_valid)
            {
                char *ip_add = inet_ntoa(response_addr.sin_addr);
                // logger->info("PKT FROM %s", ip_add);
                auto pkt = cfg.rep_pkt;
                pkt.push_back((cfg.discover_port >> 24) & 0xFF);
                pkt.push_back((cfg.discover_port >> 16) & 0xFF);
                pkt.push_back((cfg.discover_port >> 8) & 0xFF);
                pkt.push_back(cfg.discover_port & 0xFF);
                logger->trace("Replying to %s", ip_add);
                sendUdpPacket(ip_add, cfg.rep_port, (uint8_t *)pkt.data(), pkt.size());

            }
        }

        delete[] buffer_rx;
        cleanup_socket(fd);
    }

    UDPDiscoveryServerRunner::UDPDiscoveryServerRunner(UDPDiscoveryConfig cfg) : cfg(cfg)
    {
        discovery_th = std::thread(&UDPDiscoveryServerRunner::discovery_thread, this);
    }

    UDPDiscoveryServerRunner::~UDPDiscoveryServerRunner()
    {
        should_run = false;
        if (discovery_th.joinable())
            discovery_th.join();
    }

    std::vector<std::pair<std::string, int>> discoverUDPServers(UDPDiscoveryConfig cfg, int wait_millis)
    {
        std::vector<std::pair<std::string, int>> servers;
        bool should_wait = true;
        auto fun_rx = [&should_wait, &servers, &cfg]()
        {
#if defined(_WIN32)
            WSADATA wsa;
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            {
                logger->trace("Error in Remote SDR Discovery: Couldn't startup WSA socket!");
                return;
            }
#endif
            struct sockaddr_in recv_addr;
            int fd = -1;

            if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            {
                logger->trace("Error in Remote SDR Discovery: could not create socket!");
                cleanup_socket(fd);
                return;
            }

            int val_true = 1;
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val_true, sizeof(val_true)) < 0)
            {
                logger->trace("Error in Remote SDR Discovery: error setting socket options!");
                cleanup_socket(fd);
                return;
            }

            memset(&recv_addr, 0, sizeof(recv_addr));
            recv_addr.sin_family = AF_INET;
            recv_addr.sin_port = htons(cfg.rep_port);
            recv_addr.sin_addr.s_addr = INADDR_ANY;

            if (bind(fd, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) < 0)
            {
                logger->trace("Error in Remote SDR Discovery: error binding socket!");
                cleanup_socket(fd);
                return;
            }

            uint8_t *buffer_rx = new uint8_t[65536];
            while (should_wait)
            {
                struct sockaddr_in response_addr;
                socklen_t response_addr_len = sizeof(response_addr);
                int nrecv = recvfrom(fd, (char *)buffer_rx, 65536, 0, (struct sockaddr *)&response_addr, &response_addr_len);
                if (nrecv < 0)
                {
                    logger->trace("Error in Remote SDR Discovery: error on recvfrom!");
                    break;
                }

                bool is_valid = true;
                if (nrecv - 4 == (int)cfg.rep_pkt.size())
                {
                    for (size_t i = 0; i < cfg.rep_pkt.size(); i++)
                        if (buffer_rx[i] != cfg.rep_pkt[i])
                            is_valid = false;
                }
                else
                {
                    is_valid = false;
                }

                if (is_valid)
                {
                    char *ip_add = inet_ntoa(response_addr.sin_addr);
                    // logger->info("PKT FROM %s", ip_add);
                    int dport = buffer_rx[nrecv - 4] << 24 | buffer_rx[nrecv - 3] << 16 | buffer_rx[nrecv - 2] << 8 | buffer_rx[nrecv - 1];
                    servers.push_back({std::string(ip_add), dport});
                }
                
                // logger->info("EXIT %d %d %d", (int)is_valid, nrecv, cfg.rep_pkt.size());
            }

            delete[] buffer_rx;
            cleanup_socket(fd);
        };
        std::thread funrx_th(fun_rx);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        try
        {
            sendUdpBroadcast(cfg.req_port, cfg.req_pkt.data(), cfg.req_pkt.size());
        }
        catch(std::exception &e)
        {
            logger->trace("Error on Remote SDR Discovery - " + std::string(e.what()));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(wait_millis));
        should_wait = false; // Force close with a new packet
        try
        {
            sendUdpPacket("127.0.0.1", cfg.rep_port, cfg.req_pkt.data(), cfg.req_pkt.size());
        }
        catch(std::exception &e)
        {
            logger->trace("Error on Remote SDR Discovery - " + std::string(e.what()));
        }

        if (funrx_th.joinable())
            funrx_th.join();

        return servers;
    }
}
