#include "cli/ggak_merger.h"
#include "core/plugin.h"
#include "elektro_arktika/ggak/ggak.h"
#include "elektro_arktika/ggak/merger.h"
#include "explorer/explorer.h"
#include "utils/mqtt_client.h"
#include <chrono>
#include <thread>

namespace satdump
{
    void GGAKMergerCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_module = app->add_subcommand("ggak_merger", "Start a GGAK CADU stream merger");
        sub_module->add_option("server", mqtt_server);
        sub_module->add_option("port", mqtt_port);
        sub_module->add_option("satellite", satellite);
        sub_module->add_flag("--dump_cadu", dump_file);
    }

    void GGAKMergerCmdHandler::run(CLI::App *, CLI::App *subcom, bool)
    {
        std::ofstream file_out;

        if (dump_file.size())
            file_out = std::ofstream(dump_file, std::ios::binary | std::ios::app);

        elektro_arktika::ggak::GGAKMerger merger;

        MQTTClient client2(mqtt_server, mqtt_port, 1024);

        auto callback = [&](std::string, uint8_t *data, int len)
        {
            double time = *((double *)data);

            elektro_arktika::ggak::GGAKFrame *frm = ((elektro_arktika::ggak::GGAKFrame *)(data + 8));

            logger->trace("Input Frame No %d - %d", (int)frm->master_counter, (int)merger.getState());

            auto frms = merger.process(*frm);

            for (auto &f : frms)
            {
                logger->debug("Output Frame No %d!", (int)f.master_counter);

                if (dump_file.size())
                {
                    file_out.write((char *)&f, 224);
                    file_out.flush();
                }

                client2.publish("ggak/" + satellite + "/clean", (uint8_t *)&f, 224, MQTT_PUBLISH_QOS_2);
            }
        };

        MQTTClient client(mqtt_server, mqtt_port, 1024, callback);
        client.subscribe("ggak/" + satellite + "/all");

        while (1)
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }
} // namespace satdump