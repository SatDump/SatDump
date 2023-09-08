#include "remote_source.h"
#include "common/utils.h"

#include "common/net/udp_discovery.h"

void RemoteSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;
}

nlohmann::json RemoteSource::get_settings()
{

    return d_settings;
}

void RemoteSource::open()
{
    is_open = true;
}

void RemoteSource::start()
{
    DSPSampleSource::start();

    is_started = true;
}

void RemoteSource::stop()
{

    is_started = false;
}

void RemoteSource::close()
{
    if (is_open)
    {
        is_open = false;
    }
}

void RemoteSource::set_frequency(uint64_t frequency)
{
    DSPSampleSource::set_frequency(frequency);
}

void RemoteSource::drawControlUI()
{
}

void RemoteSource::set_samplerate(uint64_t samplerate)
{
}

uint64_t RemoteSource::get_samplerate()
{
    return samplerate_current;
}

std::vector<dsp::SourceDescriptor> RemoteSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    service_discovery::UDPDiscoveryConfig cfg = {REMOTE_NETWORK_DISCOVERY_PORT, REMOTE_NETWORK_DISCOVERY_REQPKT, REMOTE_NETWORK_DISCOVERY_REPPKT};

    auto detected_servers = service_discovery::discoverUDPServers(cfg, 100);

    for (auto server_ip : detected_servers)
    {
        logger->debug("Found server on %s:%d", server_ip.first.c_str(), server_ip.second);

        TCPClient tcp_client((char *)server_ip.first.c_str(), server_ip.second);

        tcp_client.readOne = true;
        tcp_client.callback_func = [&results, &server_ip](uint8_t *buf, int len)
        {
            if (buf[0] == dsp::remote::PKT_TYPE_SOURCELIST && len > 2)
            {
                std::vector<uint8_t> pkt(buf + 1, buf + len);
                for (dsp::SourceDescriptor &src : nlohmann::json::from_cbor(pkt).get<std::vector<dsp::SourceDescriptor>>())
                {
                    src.source_type = "remote";
                    src.name = server_ip.first + ":" + std::to_string(server_ip.second) + " - " + src.name;
                    results.push_back(src);
                }
            }
        };

        sendPacketWithVector(&tcp_client, dsp::remote::PKT_TYPE_SOURCELIST); // Request source list
        sleep(1);
    }

    return results;
}