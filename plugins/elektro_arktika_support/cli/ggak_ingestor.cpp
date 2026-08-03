#include "cli/ggak_ingestor.h"
#include "core/plugin.h"
#include "elektro_arktika/ggak/ggak.h"
#include "elektro_arktika/ggak/ingestor.h"
#include "elektro_arktika/ggak/merger.h"
#include "explorer/explorer.h"
#include "utils/mqtt_client.h"

namespace satdump
{
    void GGAKIngestorCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_module = app->add_subcommand("ggak_ingestor", "Start a GGAK CADU stream merger");
        sub_module->add_option("server", mqtt_server);
        sub_module->add_option("port", mqtt_port);
        sub_module->add_option("satellite", satellite);
        sub_module->add_flag("--folder", folder)->required();
    }

    void GGAKIngestorCmdHandler::run(CLI::App *, CLI::App *subcom, bool)
    {
        elektro_arktika::ggak::GGAKIngestor ingestor(folder);

        auto callback = [&](std::string topic, uint8_t *data, int len)
        {
            auto frm = (elektro_arktika::ggak::GGAKFrame *)data;
            logger->debug("Frame %d", (int)frm->master_counter);
            ingestor.work(frm);
        };

        MQTTClient mqtt(mqtt_server, mqtt_port, 1e6, callback);

        mqtt.subscribe("ggak/" + satellite + "/clean");

        while (1)
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }
} // namespace satdump