#pragma once

#include "common/net/tcp.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include "rotator_handler.h"

class RotctlHandler : public RotatorHandler
{
private:
    net::TCPClient *client = nullptr;

    char input_address[100] = "127.0.0.1";
    int input_port = 4533;

    const int MAX_CORRUPTED_CMD = 3;
    int corrupted_cmd_count = 0;

private:
    std::string command(std::string cmd, int *ret_sz)
    {
        client->sends((uint8_t *)cmd.data(), cmd.size());

        std::string result;
        result.resize(1000);

        *ret_sz = client->recvs((uint8_t *)result.data(), result.size());

        if (*ret_sz < 0)
            return "";

        result.resize(*ret_sz);
        return result;
    }

    void connect(char *address, int port)
    {
        if (client != nullptr)
            delete client;
        client = nullptr;

        try
        {
            client = new net::TCPClient(address, port);
        }
        catch (std::exception &e)
        {
            logger->error("Could not connect to Rotcld! %s", e.what());
            client = nullptr;
        }
    }

    void disconnect()
    {
        if (client != nullptr)
            delete client;
        client = nullptr;
    }

public:
    RotctlHandler()
    {
    }

    ~RotctlHandler()
    {
        if (client != nullptr)
            delete client;
        client = nullptr;
    }

    rotator_status_t get_pos(float *az, float *el)
    {
        if (client == nullptr)
            return ROT_ERROR_CON;

        float saz = 0, sel = 0;
        int ret_sz = 0;
        std::string cmd = command("p\x0a", &ret_sz);
        if (sscanf(cmd.c_str(), "%f\n%f", &saz, &sel) == 2)
        {
            corrupted_cmd_count = 0;
            *az = saz;
            *el = sel;
            return ROT_ERROR_OK;
        }

        corrupted_cmd_count++;
        if (corrupted_cmd_count > MAX_CORRUPTED_CMD || ret_sz <= 0)
        {
            if (client != nullptr)
                delete client;
            client = nullptr;
            corrupted_cmd_count = 0;
        }

        return ROT_ERROR_CON;
    }

    rotator_status_t set_pos(float az, float el)
    {
        if (client == nullptr)
            return ROT_ERROR_CON;

        char command_out[30];
        sprintf(command_out, "P %.2f %.2f\x0a", az, el);
        int ret_sz = 0;
        std::string cmd = command(std::string(command_out), &ret_sz);
        int result = 0;
        if (sscanf(cmd.c_str(), "RPRT %d", &result) == 1)
        {
            corrupted_cmd_count = 0;
            if (result != 0)
                return ROT_ERROR_CMD;
            else
                return ROT_ERROR_OK;
        }

        corrupted_cmd_count++;
        if (corrupted_cmd_count > MAX_CORRUPTED_CMD || ret_sz <= 0)
        {
            if (client != nullptr)
                delete client;
            client = nullptr;
            corrupted_cmd_count = 0;
        }

        return ROT_ERROR_CON;
    }

    void render()
    {
        if (client != nullptr)
            style::beginDisabled();
        ImGui::InputText("Address##rotctladdress", input_address, 100);
        ImGui::InputInt("Port##rotctlport", &input_port);
        if (client != nullptr)
            style::endDisabled();

        if (client != nullptr)
        {
            if (ImGui::Button("Disconnect##rotctldisconnect"))
                disconnect();
        }
        else
        {
            if (ImGui::Button("Connect##rotctlconnect"))
                connect(input_address, input_port);
        }
    }

    bool is_connected()
    {
        return client != nullptr;
    }
};