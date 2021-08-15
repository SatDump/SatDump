#pragma once

#include "sdr.h"
#include <thread>
#include <fstream>
#include "common/dsp/file_source.h"

class SDRFile : public SDRDevice
{
private:
    std::thread workThread;
    void runThread();
    bool should_run;
    std::mutex file_mutex;
    float file_progress = 0;
    uint64_t filesize;
    std::ifstream input_file;
    dsp::BasebandType baseband_type_e;

    uint64_t getFilesize(std::string filepath)
    {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        std::uint64_t fileSize = file.tellg();
        file.close();
        return fileSize;
    }

public:
    SDRFile(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRFile();
    std::map<std::string, std::string> getParameters();
    std::string getID();
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(float frequency);
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
    static int baseband_type_option;
    static std::string baseband_format;
    static char file_path[100];
    static std::map<std::string, std::string> drawParamsUI();
};
