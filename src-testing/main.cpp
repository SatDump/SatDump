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
#include "common/dsp_source_sink/remote.h"
#include "common/net/udp_discovery.h"

#include <unistd.h>

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/dsp_source_sink/dsp_sample_sink.h"

#include "common/net/tcp_test.h"

// The TCP Server
TCPServer *tcp_server;

void tcp_rx_handler(uint8_t *buffer, int len)
{
    int pkt_type = buffer[0];

    if (pkt_type == dsp::remote::PKT_TYPE_PING)
    { // Simply reply
        sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_PING);
        logger->debug("Ping!");
    }

    if (pkt_type == dsp::remote::PKT_TYPE_SOURCELIST)
    {
        std::vector<dsp::SourceDescriptor> sources = dsp::getAllAvailableSources(true);

        logger->trace("Found devices (sources) :");
        for (dsp::SourceDescriptor src : sources)
            logger->trace("- " + src.name);

        // TODO SIMPLIFY TO BINARY?
        sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_SOURCELIST, nlohmann::json::to_cbor(nlohmann::json(sources)));
    }
}

int main(int argc, char *argv[])
{
    initLogger();

    int port_used = 7887;

    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    logger->set_level(slog::LOG_TRACE);

    dsp::registerAllSources();
    dsp::registerAllSinks();

    // Start UDP discovery system
    service_discovery::UDPDiscoveryConfig cfg = {REMOTE_NETWORK_DISCOVERY_PORT, REMOTE_NETWORK_DISCOVERY_REQPKT, REMOTE_NETWORK_DISCOVERY_REPPKT, port_used};
    service_discovery::UDPDiscoveryServerRunner runner(cfg);

    // Start the main TCP Server
    tcp_server = new TCPServer(port_used);
    tcp_server->callback_func = tcp_rx_handler;

    while (1)
        sleep(1);
    return 0;
}