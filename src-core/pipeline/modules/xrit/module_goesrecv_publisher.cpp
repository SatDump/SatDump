#include "module_goesrecv_publisher.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>

#define FRAME_SIZE 1024

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace satdump
{
    namespace pipeline
    {
        namespace xrit
        {
            GOESRecvPublisherModule::GOESRecvPublisherModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
                : base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), address(parameters["address"].get<std::string>()), port(parameters["nanomsg_port"].get<int>())
            {
                buffer = new uint8_t[FRAME_SIZE];
                fsfsm_enable_output = false;
            }

            std::vector<ModuleDataType> GOESRecvPublisherModule::getInputTypes() { return {DATA_FILE, DATA_STREAM}; }

            std::vector<ModuleDataType> GOESRecvPublisherModule::getOutputTypes() { return {DATA_FILE}; }

            GOESRecvPublisherModule::~GOESRecvPublisherModule() { delete[] buffer; }

            void GOESRecvPublisherModule::process()
            {
                nng_socket sock;
                nng_listener listener;

                logger->info("Opening TCP socket on " + std::string("tcp://" + address + ":" + std::to_string(port)));

                nng_pub0_open_raw(&sock);
                nng_listener_create(&listener, sock, std::string("tcp://" + address + ":" + std::to_string(port)).c_str());
                nng_listener_start(listener, NNG_FLAG_NONBLOCK);

                while (should_run())
                {
                    // Read a buffer
                    read_data(buffer, FRAME_SIZE);

                    nng_send(sock, &buffer[4], 892, NNG_FLAG_NONBLOCK);
                }

                nng_listener_close(listener);

                cleanup();
            }

            void GOESRecvPublisherModule::drawUI(bool window)
            {
                ImGui::Begin("xRIT GOESRECV Publisher", NULL, window ? 0 : NOWINDOW_FLAGS);

                ImGui::Text("Address  : ");
                ImGui::SameLine();
                ImGui::TextColored(style::theme.green, "%s", address.c_str());

                ImGui::Text("Port    : ");
                ImGui::SameLine();
                ImGui::TextColored(style::theme.green, UITO_C_STR(port));

                drawProgressBar();

                ImGui::End();
            }

            std::string GOESRecvPublisherModule::getID() { return "xrit_goesrecv_publisher"; }

            std::shared_ptr<ProcessingModule> GOESRecvPublisherModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            {
                return std::make_shared<GOESRecvPublisherModule>(input_file, output_file_hint, parameters);
            }
        } // namespace xrit
    } // namespace pipeline
} // namespace satdump