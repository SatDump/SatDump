#include "logger.h"
#include <signal.h>
#include <filesystem>
#include "nlohmann/json.hpp"
#include <fstream>
#include "sdr/sdr.h"
#include "init.h"
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef BUILD_ZIQ
#include "common/ziq.h"
#endif
#include "common/utils.h"

bool should_stop = false;

#ifdef BUILD_ZIQ
// ZIQ
std::shared_ptr<ziq::ziq_writer> ziqWriter;
bool enable_ziq = false;
long long int compressedSamples = 0;
#endif
long long int recordedSize = 0;
std::ofstream data_out;

#ifndef _WIN32
// SIGINT Handler
void sigint_handler(int /*s*/)
{
    should_stop = true;
}
#endif

float clampF(float c)
{
    if (c > 1.0f)
        c = 1.0f;
    else if (c < -1.0f)
        c = -1.0f;
    return c;
}

int main(int argc, char *argv[])
{
    // SatDump init
    initLogger();
    //initSatdump(); // We do NOT need everything in SatDuump to be usable for recording

#ifndef _WIN32
    // Setup SIGINT handler
    struct sigaction siginthandler;
    siginthandler.sa_handler = sigint_handler;
    sigemptyset(&siginthandler.sa_mask);
    siginthandler.sa_flags = 0;
    sigaction(SIGINT, &siginthandler, NULL);
#endif

    logger->info("Starting...");
    initSDRs();

    logger->info("Found SDR Devices :");
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devices = getAllDevices();
    for (std::tuple<std::string, sdr_device_type, uint64_t> dev : devices)
        logger->info(" - [" + deviceTypeStringByType(std::get<1>(dev)) + "] " + std::get<0>(dev));

    if (argc < 5)
    {
        logger->error("Usage : ./satdump-recorder sdr_config.json samplerate_Hz frequency_MHz [i8/w8/i16/f32"
#ifdef BUILD_ZIQ
                      "/ziq8/ziq16/ziq32"
#endif
                      "] [output_file]");
        exit(1);
    }

    std::string p_sdr_cfg = argv[1];
    std::string p_samplerate = argv[2];
    std::string p_frequency = argv[3];
    std::string p_format = argv[4];
    std::string p_output_file = argc >= 6 ? argv[5] : "";

    // SDR Settings we're gonna be using
    if (!std::filesystem::exists(p_sdr_cfg))
    {
        logger->error("Could not find SDR config file " + p_sdr_cfg + "!");
        exit(1);
    }
    nlohmann::json sdr_cfg;
    {
        std::ifstream istream(p_sdr_cfg);
        istream >> sdr_cfg;
        istream.close();
    }

    sdr_device_type sdr_type = sdr_cfg.count("sdr_type") > 0 ? getDeviceIDbyIDString(sdr_cfg["sdr_type"].get<std::string>()) : NONE;
    float samplerate = std::stoi(p_samplerate);
    float frequency = std::stof(p_frequency);
    std::map<std::string, std::string> device_parameters = sdr_cfg["sdr_settings"].get<std::map<std::string, std::string>>();

    if (sdr_type == NONE)
    {
        logger->warn("SDR Type is invalid / unspecified!");
        exit(1);
    }

    // Init recording file
    const time_t timevalue = time(0);
    std::tm *timeReadable = gmtime(&timevalue);
    std::string timestamp =
        (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
        (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
        (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

#ifdef BUILD_ZIQ
    ziq::ziq_cfg cfg;
    enable_ziq = false;
#endif

    int format_type = 0;

    std::string formatstr = "";
    if (p_format == "i8")
    {
        formatstr = "i8";
        format_type = 0;
    }
    else if (p_format == "i16")
    {
        formatstr = "i16";
        format_type = 1;
    }
    else if (p_format == "f32")
    {
        formatstr = "f32";
        format_type = 2;
    }
#ifdef BUILD_ZIQ
    else if (p_format == "ziq8")
    {
        formatstr = "ziq";
        cfg.is_compressed = true;
        cfg.bits_per_sample = 8;
        cfg.samplerate = samplerate;
        cfg.annotation = "";
        enable_ziq = true;
        format_type = 4;
    }
    else if (p_format == "ziq16")
    {
        formatstr = "ziq";
        cfg.is_compressed = true;
        cfg.bits_per_sample = 16;
        cfg.samplerate = samplerate;
        cfg.annotation = "";
        enable_ziq = true;
        format_type = 5;
    }
    else if (p_format == "ziq32")
    {
        formatstr = "ziq";
        cfg.is_compressed = true;
        cfg.bits_per_sample = 32;
        cfg.samplerate = samplerate;
        cfg.annotation = "";
        enable_ziq = true;
        format_type = 5;
    }
#endif
    else
        logger->critical("Something went horribly wrong with sample format!");

    if (p_output_file == "") // If empty, chose a filename
    {
        p_output_file = "./" + timestamp + "_" + std::to_string((long)samplerate) + "SPS_" +
                        std::to_string((long)frequency * 1e6) + "Hz." + formatstr;
    }

    data_out = std::ofstream(p_output_file, std::ios::binary);
#ifdef BUILD_ZIQ
    if (enable_ziq)
        ziqWriter = std::make_shared<ziq::ziq_writer>(cfg, data_out);
    compressedSamples = 0;
#endif
    recordedSize = 0;

    logger->info("Output file " + p_output_file);

    // Init the device
    std::string devID = sdr_cfg["sdr_type"].get<std::string>();
    logger->debug("Device parameters " + devID + ":");
    for (const std::pair<std::string, std::string> param : device_parameters)
        logger->debug("   - " + param.first + " : " + param.second);

    std::shared_ptr<SDRDevice> radio;

    {
        bool found = false;
        for (int i = 0; i < (int)devices.size(); i++)
        {
            std::tuple<std::string, sdr_device_type, uint64_t> &devListing = devices[i];
            if (std::get<1>(devListing) == sdr_type)
            {
                radio = getDeviceByID(devices, device_parameters, i);
                found = true;
            }
        }

        if (!found)
        {
            logger->error("Could not find requested SDR Device!");
            exit(1);
        }
    }

    radio->setFrequency(frequency * 1e6);
    radio->setSamplerate(samplerate);
    radio->start();

    int8_t *converted_buffer_i8 = nullptr;
    int16_t *converted_buffer_i16 = nullptr;
    if (format_type == 0)
        converted_buffer_i8 = new int8_t[100000000];
    if (format_type == 1)
        converted_buffer_i16 = new int16_t[100000000];

    logger->info("Recording...");

    time_t lastTime = 0;
    while (!should_stop)
    {
        int cnt = radio->output_stream->read();

        if (cnt > 0)
        {
            // Should probably add an AGC here...
            // Also maybe some buffering but as of now it's been doing OK.
            for (int i = 0; i < cnt; i++)
            {
                // Clamp samples
                radio->output_stream->readBuf[i] = complex_t(clampF(radio->output_stream->readBuf[i].real),
                                                             clampF(radio->output_stream->readBuf[i].imag));
            }

            // This is faster than a case
            if (format_type == 0)
            {
                volk_32f_s32f_convert_8i(converted_buffer_i8, (float *)radio->output_stream->readBuf, 127, cnt * 2); // Scale to 8-bits
                data_out.write((char *)converted_buffer_i8, cnt * 2 * sizeof(uint8_t));
                recordedSize += cnt * 2 * sizeof(uint8_t);
            }
            else if (format_type == 1)
            {
                volk_32f_s32f_convert_16i(converted_buffer_i16, (float *)radio->output_stream->readBuf, 65535, cnt * 2); // Scale to 16-bits
                data_out.write((char *)converted_buffer_i16, cnt * 2 * sizeof(uint16_t));
                recordedSize += cnt * 2 * sizeof(uint16_t);
            }
            else if (format_type == 2)
            {
                data_out.write((char *)radio->output_stream->readBuf, cnt * 2 * sizeof(float));
                recordedSize += cnt * 2 * sizeof(float);
            }
#ifdef BUILD_ZIQ
            else if (enable_ziq)
            {
                compressedSamples += ziqWriter->write(radio->output_stream->readBuf, cnt);
                recordedSize += cnt * 2 * sizeof(uint8_t);
            }
#endif
        }

        radio->output_stream->flush();

        if (time(NULL) % 10 == 0 && lastTime != time(NULL))
        {
            lastTime = time(NULL);
            std::string finalstr = "";
            std::string datasize = (recordedSize > 1e9 ? to_string_with_precision<float>(recordedSize / 1e9, 2) + " GB" : to_string_with_precision<float>(recordedSize / 1e6, 2) + " MB");
#ifdef BUILD_ZIQ
            if (enable_ziq)
            {
                std::string compressedsize = (compressedSamples > 1e9 ? to_string_with_precision<float>(compressedSamples / 1e9, 2) + " GB" : to_string_with_precision<float>(compressedSamples / 1e6, 2) + " MB");
                finalstr = "Size : " + datasize + ", Compressed : " + compressedsize;
            }
            else
#endif
            {
                finalstr = "Size : " + datasize;
            }
            logger->info(finalstr);
        }
    }

    if (format_type == 0)
        delete[] converted_buffer_i8;
    if (format_type == 1)
        delete[] converted_buffer_i16;

    logger->info("Stop recording...");
#ifdef BUILD_ZIQ
    if (enable_ziq)
        ziqWriter.reset();
#endif
    data_out.close();

    logger->info("Stop SDR...");
    radio->stop();

    logger->info("Done! Goodbye");
}