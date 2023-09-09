#pragma once

#include <cstdint>
#include <vector>
#include <thread>
#include <string>

namespace service_discovery
{
    struct UDPDiscoveryConfig
    {
        int port;
        std::vector<uint8_t> req_pkt;
        std::vector<uint8_t> rep_pkt;
        uint32_t discover_port = 0;
    };

    class UDPDiscoveryServerRunner
    {
    private:
        const UDPDiscoveryConfig cfg;

        bool should_run = true;
        void discovery_thread();
        std::thread discovery_th;

    public:
        UDPDiscoveryServerRunner(UDPDiscoveryConfig cfg);
        ~UDPDiscoveryServerRunner();
    };

    std::vector<std::pair<std::string, int>> discoverUDPServers(UDPDiscoveryConfig cfg, int wait_millis);
}