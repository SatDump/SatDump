#include "file_source.h"
#include "common/utils.h"
#include "imgui/imgui_stdlib.h"

void FileSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    buffer_size = getValueOrDefault(d_settings["buffer_size"], buffer_size);
    file_path = getValueOrDefault(d_settings["file_path"], file_path);
    baseband_type = getValueOrDefault(d_settings["baseband_type"], baseband_type);
}

nlohmann::json FileSource::get_settings(nlohmann::json)
{
    d_settings["buffer_size"] = buffer_size;
    d_settings["file_path"] = file_path;
    d_settings["baseband_type"] = baseband_type;

    return d_settings;
}

void FileSource::run_thread()
{
    while (should_run)
    {
        if (is_started)
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
                input_file.read((char *)output_stream->writeBuf, buffer_size * sizeof(complex_t));
                break;

            case dsp::INTEGER_16:
                input_file.read((char *)buffer_i16, buffer_size * sizeof(int16_t) * 2);
                volk_16i_s32f_convert_32f_u((float *)output_stream->writeBuf, (const int16_t *)buffer_i16, 65535, buffer_size * 2);
                break;

            case dsp::INTEGER_8:
                input_file.read((char *)buffer_i8, buffer_size * sizeof(int8_t) * 2);
                volk_8i_s32f_convert_32f_u((float *)output_stream->writeBuf, (const int8_t *)buffer_i8, 127, buffer_size * 2);
                break;

            case dsp::WAV_8:
                input_file.read((char *)buffer_u8, buffer_size * sizeof(uint8_t) * 2);
                for (int i = 0; i < buffer_size; i++)
                {
                    float imag = (buffer_u8[i * 2 + 1] - 127) * (1.0 / 127.0);
                    float real = (buffer_u8[i * 2 + 0] - 127) * (1.0 / 127.0);
                    output_stream->writeBuf[i] = complex_t(real, imag);
                }
                break;

            default:
                break;
            }

            output_stream->swap(buffer_size);
            file_progress = (float(input_file.tellg()) / float(filesize)) * 100;
            file_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::nanoseconds(int(ns_to_wait)));
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void FileSource::open()
{
    buffer_i16 = new int16_t[buffer_size * 2];
    buffer_i8 = new int8_t[buffer_size * 2];
    buffer_u8 = new uint8_t[buffer_size * 2];
    is_open = true;
}

void FileSource::start()
{
    DSPSampleSource::start();
    ns_to_wait = (1e9 / current_samplerate) * float(buffer_size);
    filesize = getFilesize(file_path);
    input_file = std::ifstream(file_path, std::ios::binary);
    logger->debug("Opening {:s} filesize {:d}", file_path.c_str(), filesize);
    baseband_type_e = dsp::BasebandTypeFromString(baseband_type);
    is_started = true;
}

void FileSource::stop()
{
    is_started = false;
}

void FileSource::close()
{
    if (is_open)
    {
        buffer_i16 = new int16_t[buffer_size * 100];
        buffer_i8 = new int8_t[buffer_size * 100];
        buffer_u8 = new uint8_t[buffer_size * 100];
    }
}

void FileSource::set_frequency(uint64_t frequency)
{
    DSPSampleSource::set_frequency(frequency);
}

void FileSource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();
    ImGui::InputText("File", &file_path);
    ImGui::InputInt("Samplerate", &current_samplerate);
    if (ImGui::Combo("Format", &select_sample_format, "f32\0"
                                                      "s16\0"
                                                      "s8\0"))
    {
        if (select_sample_format == 0)
            baseband_type = "f32";
        else if (select_sample_format == 1)
            baseband_type = "s16";
        else if (select_sample_format == 2)
            baseband_type = "s8";
    }
    if (is_started)
        style::endDisabled();

    if (!is_started)
        style::beginDisabled();
    if (ImGui::SliderFloat("Progress", &file_progress, 0, 100))
    {
        file_mutex.lock();
        int samplesize = sizeof(complex_t);

        switch (baseband_type_e)
        {
        case dsp::COMPLEX_FLOAT_32:
            samplesize = sizeof(complex_t);
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
    if (!is_started)
        style::endDisabled();
}

void FileSource::set_samplerate(uint64_t samplerate)
{
    current_samplerate = samplerate;
}

uint64_t FileSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> FileSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    results.push_back({"file", "File Source", 0});

    return results;
}