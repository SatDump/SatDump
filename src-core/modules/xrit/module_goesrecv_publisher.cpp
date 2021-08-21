#include "module_goesrecv_publisher.h"
#include "logger.h"
#include "imgui/imgui.h"
#ifndef __ANDROID__
#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#endif

#define FRAME_SIZE 1024

// Return filesize
size_t getFilesize(std::string filepath);

namespace xrit
{
    GOESRecvPublisherModule::GOESRecvPublisherModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                            address(parameters["address"]),
                                                                                                                                                            port(std::stoi(parameters["port"]))
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

#ifndef __ANDROID__
        nng_socket sock;
        nng_listener listener;

        logger->info("Opening TCP socket on " + std::string("tcp://" + address + ":" + std::to_string(port)));

        nng_pub0_open_raw(&sock);
        nng_listener_create(&listener, sock, std::string("tcp://" + address + ":" + std::to_string(port)).c_str());
        nng_listener_start(listener, NNG_FLAG_NONBLOCK);
#endif

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)buffer, FRAME_SIZE);
            else
                input_fifo->read((uint8_t *)buffer, FRAME_SIZE);

#ifndef __ANDROID__
            nng_send(sock, &buffer[4], 892, NNG_FLAG_NONBLOCK);
#endif

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

#ifndef __ANDROID__
        nng_listener_close(listener);
#endif

        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void GOESRecvPublisherModule::drawUI(bool window)
    {
        ImGui::Begin("xRIT GOESRECV Publisher", NULL, window ? NULL : NOWINDOW_FLAGS);

        ImGui::Text("Address  : ");
        ImGui::SameLine();
        ImGui::TextColored(IMCOLOR_SYNCED, "%s", address.c_str());

        ImGui::Text("Port    : ");
        ImGui::SameLine();
        ImGui::TextColored(IMCOLOR_SYNCED, UITO_C_STR(port));

#ifdef __ANDROID__
        ImGui::TextColored(IMCOLOR_NOSYNC, "This module is not yet supported on Android due to nng compatibility issues.");
#endif

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

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

    std::shared_ptr<ProcessingModule> GOESRecvPublisherModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<GOESRecvPublisherModule>(input_file, output_file_hint, parameters);
    }
}