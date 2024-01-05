#include "module_goesrecv_publisher.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>

#define FRAME_SIZE 1024

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace xrit
{
    GOESRecvPublisherModule::GOESRecvPublisherModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                        address(parameters["address"].get<std::string>()),
                                                                                                                                        port(parameters["port"].get<int>())
    {
        buffer = new uint8_t[FRAME_SIZE];
    }

    std::vector<ModuleDataType> GOESRecvPublisherModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> GOESRecvPublisherModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    GOESRecvPublisherModule::~GOESRecvPublisherModule()
    {
        delete[] buffer;
    }

    void GOESRecvPublisherModule::process()
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
                data_in.read((char *)buffer, FRAME_SIZE);
            else
                input_fifo->read((uint8_t *)buffer, FRAME_SIZE);

            nng_send(sock, &buffer[4], 892, NNG_FLAG_NONBLOCK);

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

    void GOESRecvPublisherModule::drawUI(bool window)
    {
        ImGui::Begin("xRIT GOESRECV Publisher", NULL, window ? 0 : NOWINDOW_FLAGS);

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

    std::string GOESRecvPublisherModule::getID()
    {
        return "xrit_goesrecv_publisher";
    }

    std::vector<std::string> GOESRecvPublisherModule::getParameters()
    {
        return {"address", "port"};
    }

    std::shared_ptr<ProcessingModule> GOESRecvPublisherModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<GOESRecvPublisherModule>(input_file, output_file_hint, parameters);
    }
}