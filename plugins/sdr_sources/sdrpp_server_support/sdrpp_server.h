#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "sdrpp_server/sdrpp_server_client.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"

class SDRPPServerSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_connected = false, is_started = false;
    server::Client client;

    int selected_bit_depth = 0;

    std::string ip_address = "0.0.0.0";
    int port = 5259;
    int bit_depth = 32;
    bool compression = false;

    std::string error;

    std::shared_ptr<dsp::stream<uint8_t>> client_output_stream;

    void try_connect()
    {
        if (!client_output_stream)
            client_output_stream = std::make_shared<dsp::stream<uint8_t>>();
        client = server::connect(ip_address, port, client_output_stream.get());
        if (client == NULL)
            throw std::runtime_error("Connection error!");

        // if (!client->waitForDevInfo(4000))
        //     throw std::runtime_error("Didn't get dev info!");

        is_connected = true;
    }

    void disconnect()
    {
        if (is_started)
            stop();
        if (is_connected)
            client->close();
        is_connected = false;
    }

protected:
    std::thread convertThread;

    bool convertShouldRun = false;

    int process(int count, const uint8_t *in, complex_t *out)
    {
        uint16_t sampleType = *(uint16_t *)&in[2];
        float scaler = *(float *)&in[4];
        const void *dataBuf = &in[8];

        if (sampleType == dsp::compression::PCMType::PCM_TYPE_F32)
        {
            memcpy(out, dataBuf, count - 8);
            return (count - 8) / sizeof(complex_t);
        }
        else if (sampleType == dsp::compression::PCMType::PCM_TYPE_I16)
        {
            int outCount = (count - 8) / (sizeof(int16_t) * 2);
            volk_16i_s32f_convert_32f((float *)out, (int16_t *)dataBuf, 32768.0f / scaler, outCount * 2);
            return outCount;
        }
        else if (sampleType == dsp::compression::PCMType::PCM_TYPE_I8)
        {
            int outCount = (count - 8) / (sizeof(int8_t) * 2);
            volk_8i_s32f_convert_32f((float *)out, (int8_t *)dataBuf, 128.0f / scaler, outCount * 2);
            return outCount;
        }

        return 0;
    }

    void convertFunction()
    {
        while (convertShouldRun)
        {
            int count = client_output_stream->read();
            int outCount = process(count, client_output_stream->readBuf, output_stream->writeBuf);
            client_output_stream->flush();
            output_stream->swap(outCount);
        }
    }

    void set_params()
    {
        if (is_connected)
        {
            client->setCompression(compression);

            if (bit_depth == 32)
                client->setSampleType(dsp::compression::PCM_TYPE_F32);
            else if (bit_depth == 16)
                client->setSampleType(dsp::compression::PCM_TYPE_I16);
            else if (bit_depth == 8)
                client->setSampleType(dsp::compression::PCM_TYPE_I8);
        }
    }

public:
    SDRPPServerSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
    }

    ~SDRPPServerSource()
    {
        stop();
        close();
    }

    void set_settings(nlohmann::json settings);
    nlohmann::json get_settings();

    void open();
    void start();
    void stop();
    void close();

    void set_frequency(uint64_t frequency);

    void drawControlUI();

    void set_samplerate(uint64_t samplerate);
    uint64_t get_samplerate();

    static std::string getID() { return "sdrpp_server"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<SDRPPServerSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};