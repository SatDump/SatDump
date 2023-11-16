#include "module_s2_ts2tcp.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/codings/dvb-s2/bbframe_ts_parser.h"
#include "common/utils.h"

#if !(defined(__APPLE__) || defined(_WIN32))
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#endif

namespace dvbs2
{
    S2TStoTCPModule::S2TStoTCPModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        port = d_parameters["port"].get<int>();
        bbframe_size = d_parameters["bb_size"].get<int>();
    }

    std::vector<ModuleDataType> S2TStoTCPModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> S2TStoTCPModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    S2TStoTCPModule::~S2TStoTCPModule()
    {
    }

    void S2TStoTCPModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        logger->info("Using input bbframes " + d_input_file);

        time_t lastTime = 0;

        uint8_t *bb_buffer = new uint8_t[bbframe_size];
        uint8_t ts_frames[188 * 1000];

        dvbs2::BBFrameTSParser ts_extractor(bbframe_size);

#if !(defined(__APPLE__) || defined(_WIN32))
        // Init TCP Server
        int sockfd, clientfd, len;
        struct sockaddr_in servaddr, cli;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd == -1)
            logger->error("Couldn't create socket!");
        else
            logger->info("Socket created!");

        bzero(&servaddr, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);

        if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
            logger->error("Couldn't bind socket!");
        else
            logger->info("Socket bind!");

        if ((listen(sockfd, 5)) != 0)
            logger->error("Couldn't start listening!");
        else
            logger->info("Server started!");

        len = sizeof(cli);

        clientfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len);
        if (clientfd < 0)
            logger->error("Couldn't accept client!");
        else
            logger->info("Client accepted!");

//  std::ofstream file_ts("test_ts.ts");
#endif

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)bb_buffer, bbframe_size / 8);
            else
                input_fifo->read((uint8_t *)bb_buffer, bbframe_size / 8);

            int ts_cnt = ts_extractor.work(bb_buffer, 1, ts_frames, 188 * 1000);

            for (int i = 0; i < ts_cnt; i++)
            {
#if !(defined(__APPLE__) || defined(_WIN32))
                send(clientfd, &ts_frames[i * 188], 188, MSG_NOSIGNAL);
                // file_ts.write((char *)&ts_frames[i * 188], 188);
#endif
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

#if !(defined(__APPLE__) || defined(_WIN32))
        close(sockfd);
#endif

        delete[] bb_buffer;

        data_out.close();
        if (output_data_type == DATA_FILE)
            data_in.close();
    }

    void S2TStoTCPModule::drawUI(bool window)
    {
        ImGui::Begin("DVB-S2 TS to TCP", NULL, window ? 0 : NOWINDOW_FLAGS);
        ImGui::BeginGroup();
        {
            // ImGui::Button("TS Status", {200 * ui_scale, 20 * ui_scale});
            //{
            //     ImGui::Text("PID  : ");
            //     ImGui::SameLine();
            //     ImGui::TextColored(pid_to_decode == current_pid ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(current_pid));
            // }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string S2TStoTCPModule::getID()
    {
        return "s2_ts2tcp";
    }

    std::vector<std::string> S2TStoTCPModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> S2TStoTCPModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<S2TStoTCPModule>(input_file, output_file_hint, parameters);
    }
}