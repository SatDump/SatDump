#include "core/plugin.h"
#include "logger.h"

#include "recorder/recorder.h"
#include "imgui/imgui.h"

#include <sys/types.h>
#include <stdexcept>
#include <cstring>
#if defined(_WIN32)
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace bochum
{
    void sendUdpBroadcast(int port, uint8_t *data, int len)
    {
#if defined(_WIN32)
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            throw std::runtime_error("Couldn't startup WSA socket!");
#endif

        struct sockaddr_in send_addr;
        int fd = -1;

        if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            throw std::runtime_error("Error creating socket!");

        int val_true = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char *)&val_true, sizeof(val_true)) < 0)
            throw std::runtime_error("Error setting socket option!");

        memset(&send_addr, 0, sizeof(send_addr));
        send_addr.sin_family = AF_INET;
        send_addr.sin_port = htons(port);
        send_addr.sin_addr.s_addr = INADDR_BROADCAST;

        if (sendto(fd, (const char *)data, len, 0, (struct sockaddr *)&send_addr, sizeof(sockaddr)) < 0)
            throw std::runtime_error(std::strerror(errno));

#if defined(_WIN32)
        closesocket(fd);
        WSACleanup();
#else
        close(fd);
#endif
    }

    nlohmann::json bochum_cfg;
    std::thread udprx_th;

    void handleBochumPkt(std::string cmd);
    void udpReceiveThread()
    {
        const int internal_port = 10001;

#if defined(_WIN32)
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            throw satdump_exception("Couldn't startup WSA socket!");
#endif

        struct sockaddr_in recv_addr;
        int fd = -1;

        if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            throw satdump_exception("Error creating socket!");

        int val_true = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&val_true, sizeof(val_true)) < 0)
            throw satdump_exception("Error setting socket option!");

        memset(&recv_addr, 0, sizeof(recv_addr));
        recv_addr.sin_family = AF_INET;
        recv_addr.sin_port = htons(internal_port);
        recv_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(fd, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0)
            throw satdump_exception("Error binding socket!");

        uint8_t buffer_rx[65536];

        while (true)
        {
            struct sockaddr_in response_addr;
            socklen_t response_addr_len = sizeof(response_addr);
            int nrecv = recvfrom(fd, (char *)buffer_rx, 65536, 0, (struct sockaddr *)&response_addr, &response_addr_len);
            if (nrecv < 0)
                throw satdump_exception("Error on recvfrom!");

            try
            {
                std::string command = (char *)buffer_rx;

                if (bochum_cfg.contains("rebroadcast_port"))
                    sendUdpBroadcast(bochum_cfg["rebroadcast_port"].get<int>(), buffer_rx, nrecv);

                handleBochumPkt(command);
            }
            catch (std::exception &e)
            {
                logger->error("Error on packet! %s", e.what());
            }
        }

#if defined(_WIN32)
        closesocket(fd);
        WSACleanup();
#else
        close(fd);
#endif
    }

    /////////////////////////////////////////////////////////////////

    bool d_enabled = false;
    std::string curr_name = "";
    double curr_freq = 0;
    std::mutex ui_mtx;

    void handleBochumPkt(std::string cmd)
    {
        if (!d_enabled)
            return;

        std::string name = cmd.substr(20, 11);
        double freq = std::stod(cmd.substr(69, 11)) * 1e6;

        if (curr_name != name)
        {
            logger->warn("[Bochum] Set object to %s", name.c_str());

            nlohmann::json final_entry = bochum_cfg["default"];
            for (auto &entry : bochum_cfg.items())
                if (name.find(entry.key()) != std::string::npos)
                    final_entry = entry.value();

            satdump::eventBus->fire_event<satdump::RecorderStopProcessingEvent>({});
            satdump::eventBus->fire_event<satdump::RecorderStopDeviceEvent>({});

            if (final_entry.contains("samplerate"))
                satdump::eventBus->fire_event<satdump::RecorderSetDeviceSamplerateEvent>({final_entry["samplerate"].get<uint64_t>()});

            if (final_entry.contains("decimation"))
                satdump::eventBus->fire_event<satdump::RecorderSetDeviceDecimationEvent>({final_entry["decimation"].get<int>()});

            if (final_entry.contains("lo_offset"))
            {
                satdump::eventBus->fire_event<satdump::RecorderSetDeviceLoOffsetEvent>({final_entry["lo_offset"].get<double>() / 1e6});
            }
            else
            {
                double offset = 0;
                if (freq >= 3e9)
                    offset = bochum_cfg["default"]["xband_offset"];
                satdump::eventBus->fire_event<satdump::RecorderSetDeviceLoOffsetEvent>({offset / 1e6});
            }

            if (final_entry.contains("fft_min") ||
                final_entry.contains("fft_max") ||
                final_entry.contains("fft_avgn") ||
                final_entry.contains("fft_rate") ||
                final_entry.contains("waterfall_rate") ||
                final_entry.contains("fft_size"))
            {
                satdump::RecorderSetFFTSettingsEvent evt;
                if (final_entry.contains("fft_min"))
                    evt.fft_min = final_entry["fft_min"];
                if (final_entry.contains("fft_max"))
                    evt.fft_max = final_entry["fft_max"];
                if (final_entry.contains("fft_avgn"))
                    evt.fft_avgn = final_entry["fft_avgn"];
                if (final_entry.contains("fft_rate"))
                    evt.fft_rate = final_entry["fft_rate"];
                if (final_entry.contains("waterfall_rate"))
                    evt.waterfall_rate = final_entry["waterfall_rate"];
                if (final_entry.contains("fft_size"))
                    evt.fft_size = final_entry["fft_size"];
                satdump::eventBus->fire_event<satdump::RecorderSetFFTSettingsEvent>(evt);
            }

            satdump::eventBus->fire_event<satdump::RecorderStartDeviceEvent>({});

            if (final_entry.contains("pipeline"))
            {
                satdump::eventBus->fire_event<satdump::RecorderStartProcessingEvent>({final_entry["pipeline"].get<std::string>()});
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }

            ui_mtx.lock();
            curr_name = name;
            ui_mtx.unlock();
        }

        if (abs(curr_freq - freq) > 1e6)
        {
            satdump::eventBus->fire_event<satdump::RecorderSetFrequencyEvent>({freq});
            logger->warn("[Bochum] Set frequency to %f", freq);
            ui_mtx.lock();
            curr_freq = freq;
            ui_mtx.unlock();
        }
    }

    void recorderDrawPanelEvent(const satdump::RecorderDrawPanelEvent &evt)
    {
        if (ImGui::CollapsingHeader("Bochum"))
        {
            ImGui::Checkbox("Enabled###bochumcontrolenabled", &d_enabled);
            ImGui::Text("Object : %s", curr_name.c_str());
            ImGui::Text("Frequency : %f", curr_freq);
        }
    }
};

class BochumSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "aim_support";
    }

    void init()
    {
        bochum::bochum_cfg = loadJsonFile("bochum_cfg.json");

        satdump::eventBus->register_handler<satdump::RecorderDrawPanelEvent>([](const satdump::RecorderDrawPanelEvent &evt)
                                                                             { bochum::recorderDrawPanelEvent(evt); });

        bochum::udprx_th = std::thread(&bochum::udpReceiveThread);
    }
};

PLUGIN_LOADER(BochumSupport)