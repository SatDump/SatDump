/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include "init.h"
#include "udp_discovery.h"

#include "main.h"
#include "gui.h"
#include "streaming.h"
#include "remote.h"
#include "actions.h"
#include "pkt_handler.h"

// The TCP Server
TCPServer *tcp_server;

// Currently open source & statuses
std::mutex source_mtx;
std::shared_ptr<dsp::DSPSampleSource> current_sample_source;

// Source status
bool source_is_open = false;
bool source_is_started = false;

int main(int argc, char *argv[])
{
    initLogger();
    logger->info("Starting SatDump SDR Server v" + (std::string)SATDUMP_VERSION);

    int port_used = 5656;
    try
    {
        if (argc > 1)
            port_used = std::stoi(argv[1]);
    }
    catch (std::exception&)
    {
        logger->error("Usage : " + std::string(argv[0]) + " [port]");
        return 1;
    }

    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_OFF);
    satdump::tle_do_update_on_init = false;
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    dsp::registerAllSources();
    // dsp::registerAllSinks();

    // Start UDP discovery system
    service_discovery::UDPDiscoveryConfig cfg = {REMOTE_NETWORK_DISCOVERY_REQPORT, REMOTE_NETWORK_DISCOVERY_REPPORT, REMOTE_NETWORK_DISCOVERY_REQPKT, REMOTE_NETWORK_DISCOVERY_REPPKT, (uint32_t)port_used};
    service_discovery::UDPDiscoveryServerRunner runner(cfg);

    // Start the main TCP Server
    tcp_server = new TCPServer(port_used);
    tcp_server->callback_func = tcp_rx_handler;
    tcp_server->callback_func_on_lost_client = []()
    {
        action_sourceStop();
        action_sourceClose();
    };

    // Start source GUI loop
    std::thread source_gui_th(sourceGuiThread);

    // Start source IQ loop
    std::thread source_iq_th(sourceStreamThread);

    while (1)
        tcp_server->wait_client();
    //  std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}