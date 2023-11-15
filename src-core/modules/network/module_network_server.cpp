#include "module_network_server.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>

namespace network
{
    NetworkServerModule::NetworkServerModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
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

        buffer = new uint8_t[pkt_size];
    }

    std::vector<ModuleDataType> NetworkServerModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> NetworkServerModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    NetworkServerModule::~NetworkServerModule()
    {
        delete[] buffer;
    }

    void NetworkServerModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        logger->info("Using input frames " + d_input_file);

        time_t lastTime = 0;

        nng_socket sock;
        nng_listener listener;

        logger->info("Opening TCP socket on " + std::string("tcp://" + address + ":" + std::to_string(port)));

        nng_pub0_open_raw(&sock);
        nng_listener_create(&listener, sock, std::string("tcp://" + address + ":" + std::to_string(port)).c_str());
        nng_listener_start(listener, NNG_FLAG_NONBLOCK);

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)buffer, pkt_size);
            else
                input_fifo->read((uint8_t *)buffer, pkt_size);

            nng_send(sock, buffer, pkt_size, NNG_FLAG_NONBLOCK);

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        nng_listener_close(listener);

        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void NetworkServerModule::drawUI(bool window)
    {
        ImGui::Begin("Network Server", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Text("Address  : ");
        ImGui::SameLine();
        ImGui::TextColored(IMCOLOR_SYNCED, "%s", address.c_str());

        ImGui::Text("Port    : ");
        ImGui::SameLine();
        ImGui::TextColored(IMCOLOR_SYNCED, UITO_C_STR(port));

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NetworkServerModule::getID()
    {
        return "network_server";
    }

    std::vector<std::string> NetworkServerModule::getParameters()
    {
        return {"server_address", "server_port", "pkt_size"};
    }

    std::shared_ptr<ProcessingModule> NetworkServerModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NetworkServerModule>(input_file, output_file_hint, parameters);
    }
}