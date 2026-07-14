#include "elektro_arktika/ggak/module_ggak_to_mqtt.h"
#include "imgui/imgui.h"
#include "libs/mqttc/mqtt.h"
#include "utils/mqtt_client.h"
#include "utils/time.h"
#include <cstdint>
#include <ctime>
#include <memory>

#define BUFFER_SIZE (8 + 1792)

namespace elektro_arktika
{
    namespace ggak
    {
        GGAKToMQTTModule::GGAKToMQTTModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            mqtt = std::make_unique<satdump::MQTTClient>(parameters["address"], parameters["port"], 1e6);
            fsfsm_file_ext = ".cadu";
        }

        GGAKToMQTTModule::~GGAKToMQTTModule() {}

        void GGAKToMQTTModule::process()
        {

            uint8_t frame_buffer[BUFFER_SIZE];

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)frame_buffer + 8, BUFFER_SIZE - 8);

                *((double *)frame_buffer) = satdump::getTime();

                mqtt->publish("ggak/l2/all", frame_buffer, BUFFER_SIZE, MQTT_PUBLISH_QOS_1);

                // Write it out
                write_data((uint8_t *)frame_buffer + 8, BUFFER_SIZE - 8);
            }

            cleanup();
        }

        nlohmann::json GGAKToMQTTModule::getModuleStats() { return satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats(); }

        void GGAKToMQTTModule::drawUI(bool window)
        {
            ImGui::Begin("GGAK to MQTT", NULL, window ? 0 : NOWINDOW_FLAGS);

            drawProgressBar();

            ImGui::End();
        }

        std::string GGAKToMQTTModule::getID() { return "ggak_to_mqtt"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> GGAKToMQTTModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GGAKToMQTTModule>(input_file, output_file_hint, parameters);
        }
    } // namespace ggak
} // namespace elektro_arktika