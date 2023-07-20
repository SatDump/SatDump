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
    int input_port = 4455;

private:
    std::string command(std::string cmd)
    {
        client->sends((uint8_t *)cmd.data(), cmd.size());

        std::string result;
        result.resize(1000);

        int sz = client->recvs((uint8_t *)result.data(), result.size());

        if (sz < 0)
            return "";

        //    printf("%d\n", sz);

        result.resize(sz);
        return result;
    }

    void connect(char *address, int port)
    {
        if (client != nullptr)
            delete client;
        client = nullptr;

        client = new net::TCPClient(address, port);
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

    bool get_pos(float *az, float *el)
    {
        if (client == nullptr)
            return true;

        float saz = 0, sel = 0;
        std::string cmd = command("p\x0a");
        if (sscanf(cmd.c_str(), "%f\n%f", &saz, &sel) == 2)
        {
            *az = saz;
            *el = sel;
            return false;
        }
        printf("%s\n", cmd.c_str());
        return true;
    }

    bool set_pos(float az, float el)
    {
        if (client == nullptr)
            return true;

        char command_out[30];
        sprintf(command_out, "P %.2f %.2f\x0a", az, el);
        std::string cmd = command(std::string(command_out));
        int result = 0;
        if (sscanf(cmd.c_str(), "RPRT %d", &result) == 1)
        {
            if (result != 0)
                return true;
            else
                return false;
        }
        printf("%s\n", cmd.c_str());
        return true;
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