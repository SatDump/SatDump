#include "file_source.h"
#include "common/utils.h"
#include "imgui/imgui_stdlib.h"
#include "common/wav.h"

void FileSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    iq_swap = getValueOrDefault(d_settings["iq_swap"], iq_swap);
    buffer_size = getValueOrDefault(d_settings["buffer_size"], buffer_size);
    file_path = getValueOrDefault(d_settings["file_path"], file_path);
    baseband_type = getValueOrDefault(d_settings["baseband_type"], baseband_type);
}

nlohmann::json FileSource::get_settings(nlohmann::json)
{
    d_settings["iq_swap"] = iq_swap;
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
            baseband_reader.read_samples(output_stream->writeBuf, buffer_size);

            if (iq_swap)
                for (int i = 0; i < buffer_size; i++)
                    output_stream->writeBuf[i] = complex_t(output_stream->writeBuf[i].imag, output_stream->writeBuf[i].real);

            output_stream->swap(buffer_size);
            file_progress = (float(baseband_reader.progress) / float(baseband_reader.filesize)) * 100.0;

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
    is_open = true;
}

void FileSource::start()
{
    DSPSampleSource::start();
    ns_to_wait = (1e9 / current_samplerate) * float(buffer_size);

    baseband_type_e = dsp::basebandTypeFromString(baseband_type);
    baseband_reader.set_file(file_path, baseband_type_e);
    baseband_reader.should_repeat = true;

    logger->debug("Opening {:s} filesize {:d}", file_path.c_str(), baseband_reader.filesize);

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

    bool update_format = false;

    if (file_input.draw())
    {
        file_path = file_input.getPath();

        if (std::filesystem::exists(file_path) && !std::filesystem::is_directory(file_path))
        {
            if (wav::isValidWav(wav::parseHeaderFromFileWav(file_path)))
            {
                logger->debug("File is wav!");
                current_samplerate = wav::parseHeaderFromFileWav(file_path).samplerate;
                if (wav::parseHeaderFromFileWav(file_path).bits_per_sample == 8)
                    select_sample_format = 3;
                else if (wav::parseHeaderFromFileWav(file_path).bits_per_sample == 16)
                    select_sample_format = 1;
                update_format = true;
            }
            else if (wav::isValidRF64(wav::parseHeaderFromFileWav(file_path)))
            {
                logger->debug("File is RF64!");
                current_samplerate = wav::parseHeaderFromFileRF64(file_path).samplerate;
                if (wav::parseHeaderFromFileRF64(file_path).bits_per_sample == 8)
                    select_sample_format = 2;
                else if (wav::parseHeaderFromFileRF64(file_path).bits_per_sample == 16)
                    select_sample_format = 1;
                update_format = true;
            }
#ifdef BUILD_ZIQ
            else if (ziq::isValidZIQ(file_path))
            {
                select_sample_format = 4;
                update_format = true;
                current_samplerate = ziq::getCfgFromFile(file_path).samplerate;
            }
#endif
        }
    }

    ImGui::InputInt("Samplerate", &current_samplerate);
    if (ImGui::Combo("Format###basebandplayerformat", &select_sample_format, "f32\0"
                                                                             "s16\0"
                                                                             "s8\0"
                                                                             "u8\0"
#ifdef BUILD_ZIQ
                                                                             "ziq\0"
#endif
                     ) ||
        update_format)
    {
        if (select_sample_format == 0)
            baseband_type = "f32";
        else if (select_sample_format == 1)
            baseband_type = "s16";
        else if (select_sample_format == 2)
            baseband_type = "s8";
        else if (select_sample_format == 3)
            baseband_type = "u8";
#ifdef BUILD_ZIQ
        else if (select_sample_format == 4)
            baseband_type = "ziq";
#endif
    }

    ImGui::Checkbox("IQ Swap", &iq_swap);

    if (is_started)
        style::endDisabled();

    if (!is_started)
        style::beginDisabled();
    if (ImGui::SliderFloat("Progress", &file_progress, 0, 100))
    {
        baseband_reader.set_progress(file_progress);
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