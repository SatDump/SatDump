#include "file.h"

#include "imgui/imgui.h"
#include "logger.h"

void SDRFile::runThread()
{
    int buf_size = 8192;
    float ns_to_wait = (1e9 / d_samplerate) * float(buf_size);

    // Int16 buffer
    int16_t *buffer_i16 = new int16_t[buf_size * 100];
    // Int8 buffer
    int8_t *buffer_i8 = new int8_t[buf_size * 100];
    // Uint8 buffer
    uint8_t *buffer_u8 = new uint8_t[buf_size * 100];

    while (should_run)
    {
        file_mutex.lock();
        if (input_file.eof())
        {
            input_file.clear();
            input_file.seekg(0);
        }

        switch (baseband_type_e)
        {
        case dsp::COMPLEX_FLOAT_32:
            input_file.read((char *)output_stream->writeBuf, buf_size * sizeof(std::complex<float>));
            break;

        case dsp::INTEGER_16:
            input_file.read((char *)buffer_i16, buf_size * sizeof(int16_t) * 2);
            volk_16i_s32f_convert_32f_u((float *)output_stream->writeBuf, (const int16_t *)buffer_i16, 1.0f / 0.00004f, buf_size * 2);
            break;

        case dsp::INTEGER_8:
            input_file.read((char *)buffer_i8, buf_size * sizeof(int8_t) * 2);
            volk_8i_s32f_convert_32f_u((float *)output_stream->writeBuf, (const int8_t *)buffer_i8, 1.0f / 0.004f, buf_size * 2);
            break;

        case dsp::WAV_8:
            input_file.read((char *)buffer_u8, buf_size * sizeof(uint8_t) * 2);
            for (int i = 0; i < buf_size; i++)
            {
                float imag = (buffer_u8[i * 2] - 127) * 0.004f;
                float real = (buffer_u8[i * 2 + 1] - 127) * 0.004f;
                output_stream->writeBuf[i] = std::complex<float>(real, imag);
            }
            break;

        default:
            break;
        }

        output_stream->swap(buf_size);
        file_progress = (float(input_file.tellg()) / float(filesize)) * 100;
        file_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::nanoseconds(int(ns_to_wait)));
    }

    delete[] buffer_i16;
    delete[] buffer_i8;
    delete[] buffer_u8;
}

SDRFile::SDRFile(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
    std::string baseband_type = "f32", baseband_path = "";
    if (parameters.count("file_format") > 0)
    {
        baseband_type = parameters["file_format"];
    }
    else
    {
        logger->critical("No file format specified!!! Using f32");
    }

    if (parameters.count("file_path") > 0)
    {
        baseband_path = parameters["file_path"];
    }
    else
    {
        logger->critical("No file path specified!!! Things will NOT work!!!!");
    }

    //file_source = std::make_shared<dsp::FileSourceBlock>(baseband_path, dsp::BasebandTypeFromString(baseband_type), 8192);
    filesize = getFilesize(baseband_path);
    baseband_type_e = dsp::BasebandTypeFromString(baseband_type);
    input_file = std::ifstream(baseband_path, std::ios::binary);
}

std::map<std::string, std::string> SDRFile::getParameters()
{
    return d_parameters;
}

void SDRFile::start()
{
    logger->info("Samplerate " + std::to_string(d_samplerate));

    should_run = true;
    workThread = std::thread(&SDRFile::runThread, this);
}

void SDRFile::stop()
{
    should_run = false;
    if (workThread.joinable())
        workThread.join();
}

SDRFile::~SDRFile()
{
}

void SDRFile::drawUI()
{
    ImGui::Begin("File Control", NULL);

    if (ImGui::SliderFloat("Progress", &file_progress, 0, 100))
    {
        file_mutex.lock();
        int samplesize = sizeof(std::complex<float>);

        switch (baseband_type_e)
        {
        case dsp::COMPLEX_FLOAT_32:
            samplesize = sizeof(std::complex<float>);
            break;

        case dsp::INTEGER_16:
            samplesize = sizeof(int16_t) * 2;
            break;

        case dsp::INTEGER_8:
            samplesize = sizeof(int8_t) * 2;
            break;

        case dsp::WAV_8:
            samplesize = sizeof(uint8_t) * 2;
            break;

        default:
            break;
        }

        uint64_t position = double(filesize / samplesize - 1) * (file_progress / 100.0f);
        input_file.seekg(position * samplesize);
        file_mutex.unlock();
    }

    ImGui::End();
}

void SDRFile::setFrequency(float frequency)
{
    d_frequency = frequency;
}

void SDRFile::init()
{
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRFile::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

    results.push_back({"File Source", FILESRC, 0});

    return results;
}

int SDRFile::baseband_type_option = 2;
std::string SDRFile::baseband_format = "";
char SDRFile::file_path[100] = "";

std::map<std::string, std::string> SDRFile::drawParamsUI()
{
    ImGui::Text("File path");
    ImGui::SameLine();
    ImGui::InputText("##filesourcepath", file_path, 100);

    ImGui::Text("Baseband Type");
    ImGui::SameLine();
    if (ImGui::Combo("#basebandtypefilesrc", &baseband_type_option, "f32\0i16\0i8\0w8\0"))
    {
        switch (baseband_type_option)
        {
        case 0:
            baseband_format = "f32";
            break;
        case 1:
            baseband_format = "i16";
            break;
        case 2:
            baseband_format = "i8";
            break;
        case 3:
            baseband_format = "w8";
            break;
        }
    }

    return {{"file_format", baseband_format}, {"file_path", std::string(file_path)}};
}

std::string SDRFile::getID()
{
    return "file";
}
