#include "rotcl_handler.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include "logger.h"

namespace rotator
{
    RotctlHandler::RotctlHandler()
    {
    }

    RotctlHandler::~RotctlHandler()
    {
        if (client != nullptr)
            delete client;
        client = nullptr;
    }

    std::string RotctlHandler::command(std::string cmd, int *ret_sz)
    {
        client->sends((uint8_t *)cmd.data(), cmd.size());

        std::string result;
        result.resize(1000);

        try
        {
            *ret_sz = client->recvs((uint8_t *)result.data(), result.size());
        }
        catch (std::exception &e)
        {
            logger->error(e.what());
            disconnect();
            return "";
        }

        if (*ret_sz < 0)
            return "";

        result.resize(*ret_sz);
        return result;
    }

    void RotctlHandler::l_connect(char *address, int port)
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

    void RotctlHandler::l_disconnect()
    {
        if (client != nullptr)
            delete client;
        client = nullptr;
    }

    std::string RotctlHandler::get_id()
    {
        return "rotctl";
    }

    void RotctlHandler::set_settings(nlohmann::json settings)
    {
        std::string vaddress = getValueOrDefault(settings["address"], std::string(input_address));
        memcpy(input_address, vaddress.data(), vaddress.size());
        input_port = getValueOrDefault(settings["port"], input_port);

        azLimits[0] = getValueOrDefault(settings["az_min"], 0);
        azLimits[1] = getValueOrDefault(settings["az_max"], 360);
        elLimits[0] = getValueOrDefault(settings["el_min"], 0);
        elLimits[1] = getValueOrDefault(settings["el_max"], 90);
    }

    nlohmann::json RotctlHandler::get_settings()
    {
        nlohmann::json v;
        v["address"] = std::string(input_address);
        v["port"] = input_port;
        v["az_min"] = azLimits[0];
        v["az_max"] = azLimits[1];
        v["el_min"] = elLimits[0];
        v["el_max"] = elLimits[1];
        return v;
    }

    rotator_status_t RotctlHandler::get_pos(float *az, float *el)
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

    rotator_status_t RotctlHandler::set_pos(float az, float el)
    {
        if (client == nullptr)
            return ROT_ERROR_CON;

        if (az < azLimits[0] || az > azLimits[1] || el < elLimits[0] || el > elLimits[1]) {
          return ROT_ERROR_INV_ARG;
          logger->error("Invalid azimuth or elevation value");
        }

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

    void RotctlHandler::render()
    {
        if (client != nullptr)
            style::beginDisabled();

        ImGui::InputFloat2("Azimuth Limits", azLimits.data());
        ImGui::InputFloat2("Elevation Limits", elLimits.data());
        ImGui::InputText("Address##rotctladdress", input_address, 100);
        ImGui::InputInt("Port##rotctlport", &input_port);
        if (client != nullptr)
            style::endDisabled();

        if (client != nullptr)
        {
            if (ImGui::Button("Disconnect##rotctldisconnect"))
                l_disconnect();
        }
        else
        {
            if (ImGui::Button("Connect##rotctlconnect"))
                l_connect(input_address, input_port);
        }
    }

    bool RotctlHandler::is_connected()
    {
        return client != nullptr;
    }

    void RotctlHandler::connect()
    {
        l_connect(input_address, input_port);
    }

    void RotctlHandler::disconnect()
    {
        l_disconnect();
    }
}
