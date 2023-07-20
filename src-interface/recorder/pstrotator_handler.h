#pragma once

#include "common/net/udp.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include "rotator_handler.h"

class PstRotatorHandler : public RotatorHandler
{
private:
    net::UDPClient *client = nullptr;

    char input_address[100] = "127.0.0.1";
    int input_port = 12000;

private:
    std::string command(std::string cmd)
    {
        client->send((uint8_t *)cmd.data(), cmd.size());

        std::string result;
        result.resize(1000);

        int sz = client->recv((uint8_t *)result.data(), result.size());

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

        client = new net::UDPClient(address, port);
    }

    void disconnect()
    {
        if (client != nullptr)
            delete client;
        client = nullptr;
    }

public:
    PstRotatorHandler()
    {
    }

    ~PstRotatorHandler()
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
        std::string cmd = command("<PST>AZ?</PST>\x0a");
        if (sscanf(cmd.c_str(), "AZ:%f<CR>", &saz) == 1)
        {
            *az = saz;
        }
        cmd = command("<PST>EL?</PST>\x0a");
        if (sscanf(cmd.c_str(), "EL:%f<CR>", &sel) == 1)
        {
            *el = sel;
            return false;
        }
        return true;
    }

    bool set_pos(float az, float el)
    {
        if (client == nullptr)
            return true;

        std::string cmd_az = "<PST><AZIMUTH>" + std::to_string(az) + "</AZIMUTH></PST>";
        std::string cmd_el = "<PST><EVATION>" + std::to_string(el) + "</EVATION></PST>";
        std::string rcmd_az = command(cmd_az);
        std::string rcmd_el = command(cmd_el);

        return true;
    }

    void render()
    {
        if (client != nullptr)
            style::beginDisabled();
        ImGui::InputText("Address##pstrotatoraddress", input_address, 100);
        ImGui::InputInt("Port##pstrotatorport", &input_port);
        if (client != nullptr)
            style::endDisabled();

        if (client != nullptr)
        {
            if (ImGui::Button("Disconnect##pstrotatordisconnect"))
                disconnect();
        }
        else
        {
            if (ImGui::Button("Connect##pstrotatorconnect"))
                connect(input_address, input_port);
        }
    }

    bool is_connected()
    {
        return client != nullptr;
    }
};