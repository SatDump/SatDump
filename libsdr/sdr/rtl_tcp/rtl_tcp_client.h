#pragma once

#ifndef DISABLE_SDR_RTLTCP
#include <string>
#ifdef _WIN32
#include <WinSock2.h>
#endif

class RTLTCPClient
{
private:
#ifdef _WIN32
    SOCKET socket_fd;
#else
    int socket_fd;
#endif
    bool is_connected;

public:
    RTLTCPClient();

    bool connectClient(std::string address, int port);
    void disconnect();

    void receiveSamples(uint8_t *buffer, int size);

    void sendCmd(uint8_t cmd, uint32_t prm);

    void setSamplerate(double samplerate);
    void setFrequency(double frequency);
    void setGainMode(bool mode);
    void setGain(int gain);
    void setAGCMode(bool enable);
};
#endif