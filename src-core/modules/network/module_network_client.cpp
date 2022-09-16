#include "module_network_client.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include <nng/nng.h>
#include <nng/protocol/pubsub0/sub.h>

namespace network
{
    NetworkClientModule::NetworkClientModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
        if (parameters.count("pkt_size") > 0)
            pkt_size = parameters["pkt_size"].get<int>();
        else
            throw std::runtime_error("pkt_size parameter must be present!");

        if (parameters.count("server_address") > 0)
            address = parameters["server_address"].get<std::string>();
        else
            throw std::runtime_error("server_address parameter must be present!");

        if (parameters.count("server_port") > 0)
            port = parameters["server_port"].get<int>();
        else
            throw std::runtime_error("server_port parameter must be present!");

        buffer = new uint8_t[pkt_size * 10];
    }

    std::vector<ModuleDataType> NetworkClientModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> NetworkClientModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    NetworkClientModule::~NetworkClientModule()
    {
        delete[] buffer;
    }

    void NetworkClientModule::process()
    {
        nng_socket sock;
        nng_dialer dialer;

        logger->info("Opening TCP socket on " + std::string("tcp://" + address + ":" + std::to_string(port)));

        nng_sub0_open_raw(&sock);
        nng_dialer_create(&dialer, sock, std::string("tcp://" + address + ":" + std::to_string(port)).c_str());
        nng_dialer_start(dialer, (int)0);

        while (input_active.load())
        {
            size_t lpkt_size;
            nng_recv(sock, buffer, &lpkt_size, (int)0);

            if (pkt_size != (int)lpkt_size)
                continue;

            output_fifo->write(buffer, pkt_size);
        }

        nng_dialer_close(dialer);
    }

    void NetworkClientModule::drawUI(bool window)
    {
        ImGui::Begin("Network Client", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Text("Server Address  : ");
        ImGui::SameLine();
        ImGui::TextColored(IMCOLOR_SYNCED, "%s", address.c_str());

        ImGui::Text("Server Port    : ");
        ImGui::SameLine();
        ImGui::TextColored(IMCOLOR_SYNCED, UITO_C_STR(port));

        ImGui::End();
    }

    std::string NetworkClientModule::getID()
    {
        return "network_client";
    }

    std::vector<std::string> NetworkClientModule::getParameters()
    {
        return {"server_address", "server_port", "pkt_size"};
    }

    std::shared_ptr<ProcessingModule> NetworkClientModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NetworkClientModule>(input_file, output_file_hint, parameters);
    }
}