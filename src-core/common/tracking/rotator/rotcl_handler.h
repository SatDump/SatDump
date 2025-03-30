#pragma once

#include "common/net/tcp.h"
#include "rotator_handler.h"

namespace rotator
{
    class RotctlHandler : public RotatorHandler
    {
    private:
        net::TCPClient *client = nullptr;

        char input_address[100] = "127.0.0.1";
        int input_port = 4533;

        const int MAX_CORRUPTED_CMD = 3;
        int corrupted_cmd_count = 0;

    private:
        std::string command(std::string cmd, int *ret_sz);

        void l_connect(char *address, int port);
        void l_disconnect();

    public:
        RotctlHandler();
        ~RotctlHandler();

        std::string get_id();

        void set_settings(nlohmann::json settings);
        nlohmann::json get_settings();

        rotator_status_t get_pos(float *az, float *el);
        rotator_status_t set_pos(float az, float el);

        void render();
        bool is_connected();

        void connect();
        void disconnect();
    };
}
