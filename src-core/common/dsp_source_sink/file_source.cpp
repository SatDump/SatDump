#include "logger.h"
#include "core/config.h"
#include "core/style.h"
#include "file_source.h"
#include "common/utils.h"
#include "imgui/imgui_stdlib.h"
#include "common/detect_header.h"

FileSource::FileSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
{
    file_input.setDefaultDir(satdump::config::main_cfg["satdump_directories"]["default_input_directory"]["value"].get<std::string>());
    should_run = true;
    work_thread = std::thread(&FileSource::run_thread, this);
}

FileSource::~FileSource()
{
    stop();
    close();
    should_run = false;
    if (work_thread.joinable())
        work_thread.join();
}

void FileSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;
    iq_swap = getValueOrDefault(d_settings["iq_swap"], iq_swap);
    buffer_size = getValueOrDefault(d_settings["buffer_size"], buffer_size);
    file_path = getValueOrDefault(d_settings["file_path"], file_path);
    baseband_type = getValueOrDefault(d_settings["baseband_type"], (std::string)baseband_type);
}

nlohmann::json FileSource::get_settings()
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
            int read = baseband_reader.read_samples(output_stream->writeBuf, buffer_size);

            output_stream->getDataSize();

            if (iq_swap)
                for (int i = 0; i < read; i++)
                    output_stream->writeBuf[i] = complex_t(output_stream->writeBuf[i].imag, output_stream->writeBuf[i].real);

            output_stream->swap(read);
            file_progress = (double(baseband_reader.progress) / double(baseband_reader.filesize)) * 100.0;

            if (!fast_playback)
            {
                total_samples += read;
                auto now = std::chrono::steady_clock::now();
                auto expected_time = start_time_point + sample_time_period * total_samples;

                if (expected_time < now)
                {
                    // We got behind, either because we're slow or fast
                    // mode stopped. Reset counters and carry on.
                    start_time_point = now;
                    total_samples = 0;
                }
                else
                    std::this_thread::sleep_until(expected_time);
            }
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
    if (is_ui)
        file_path = file_input.getPath();

    // Sanity Checks
    if (!std::filesystem::exists(file_path) || std::filesystem::is_directory(file_path))
        throw satdump_exception("Invalid file path " + file_path);
    if (samplerate_input.get() <= 0)
        throw satdump_exception("Invalid samplerate " + std::to_string(samplerate_input.get()));

    buffer_size = std::min<int>(dsp::STREAM_BUFFER_SIZE, std::max<int>(8192 + 1, samplerate_input.get() / 200));

    DSPSampleSource::start();
    sample_time_period = std::chrono::duration<double>(1.0 / (double)samplerate_input.get());
    start_time_point = std::chrono::steady_clock::now();
    total_samples = 0;

    baseband_reader.set_file(file_path, baseband_type);
    baseband_reader.should_repeat = true;

    logger->debug("Opening %s filesize " PRIu64, file_path.c_str(), baseband_reader.filesize);

    is_started = true;
}

void FileSource::stop()
{
    if (!is_started)
        return;

    is_started = false;
    output_stream->flush();
}

void FileSource::close()
{
    if (is_open)
    {
        is_open = false;
    }
}

void FileSource::set_frequency(uint64_t frequency)
{
    DSPSampleSource::set_frequency(frequency);
}

void FileSource::drawControlUI()
{
    is_ui = true;

    if (is_started)
        style::beginDisabled();

    if (file_input.draw())
    {
        file_path = file_input.getPath();
        if (std::filesystem::exists(file_path) && !std::filesystem::is_directory(file_path))
        {
            nlohmann::json hdr;
            try_get_params_from_input_file(hdr, file_path);
            if (hdr.contains("baseband_format"))
                baseband_type = hdr["baseband_format"].get<std::string>();

            if (hdr.contains("samplerate"))
                samplerate_input.set(hdr["samplerate"].get<uint64_t>());

            if (hdr.contains("frequency"))
                d_frequency = hdr["frequency"].get<uint64_t>();
        }
    }

    samplerate_input.draw();
    baseband_type.drawPlaybackCombo();
    ImGui::Checkbox("IQ Swap", &iq_swap);

    if (is_started)
        style::endDisabled();

    ImGui::SameLine(0.0, 15.0);
    ImGui::Checkbox("Fast", &fast_playback);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Play/demod the baseband as fast as your PC can handle it");

    if (!is_started)
        style::beginDisabled();
    if (ImGui::SliderFloat("Progress", &file_progress, 0, 100))
        baseband_reader.set_progress(file_progress);
    if (!is_started)
        style::endDisabled();
#ifdef BUILD_ZIQ
    if (baseband_type == dsp::ZIQ)
        ImGui::TextColored(style::theme.red, "ZIQ seeking may be slow!");
#endif
}

void FileSource::set_samplerate(uint64_t samplerate)
{
    samplerate_input.set(samplerate);
}

uint64_t FileSource::get_samplerate()
{
    return samplerate_input.get();
}

std::vector<dsp::SourceDescriptor> FileSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    results.push_back({"file", "File Source", "0", false});

    return results;
}
