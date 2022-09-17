#include "live.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include <signal.h>
#include "logger.h"
#include "common/cli_utils.h"
#include "common/dsp/file_sink.h"
#include "init.h"

// Catch CTRL+C to exit live properly!
bool rec_should_exit = false;
void sig_handler_rec(int signo)
{
    if (signo == SIGINT)
        rec_should_exit = true;
}

int main_record(int argc, char *argv[])
{
    if (argc < 5) // Check overall command
    {
        logger->error("Usage : " + std::string(argv[0]) + " record [output_baseband (without extension!)] [additional options as required]");
        logger->error("Extra options (examples. Any parameter used in sources can be used here) :");
        logger->error(" --samplerate [baseband_samplerate] --baseband_format [f32/s16/s8/u8/w16] --dc_block --iq_swap");
        logger->error(" --source [airspy/rtlsdr/etc] --gain 20 --bias");
        logger->error("As well as --timeout in seconds");
        logger->error("Sample command :");
        logger->error("./satdump record baseband_name --source airspy --samplerate 6e6 --frequency 1701.3e6 --general_gain 18 --bias --timeout 780");
        return 1;
    }

    // Init SatDump
    satdump::initSatdump();

    std::string output_file = argv[2];

    // Parse flags
    nlohmann::json parameters = parse_common_flags(argc - 3, &argv[3]);

    uint64_t samplerate;
    uint64_t frequency;
    uint64_t timeout;
    std::string handler_id;

    try
    {
        samplerate = parameters["samplerate"].get<uint64_t>();
        frequency = parameters["frequency"].get<uint64_t>();
        timeout = parameters.contains("timeout") ? parameters["timeout"].get<uint64_t>() : 0;
        handler_id = parameters["source"].get<std::string>();
    }
    catch (std::exception &e)
    {
        logger->error("Error parsing arguments! {:s}", e.what());
        return 1;
    }

    // Create output dir
    if (!std::filesystem::exists(output_file))
        std::filesystem::create_directories(output_file);

    // Get all sources
    dsp::registerAllSources();
    std::vector<dsp::SourceDescriptor> source_tr = dsp::getAllAvailableSources();
    dsp::SourceDescriptor selected_src;

    for (dsp::SourceDescriptor src : source_tr)
        logger->debug("Device " + src.name);

    // Try to find it and check it's usable
    bool src_found = false;
    for (dsp::SourceDescriptor src : source_tr)
    {
        if (handler_id == src.source_type)
        {
            selected_src = src;
            src_found = true;
        }
    }

    if (!src_found)
    {
        logger->error("Could not find a handler for source type : {:s}!", handler_id.c_str());
        return 1;
    }

    // Init source
    std::shared_ptr<dsp::DSPSampleSource> source_ptr = getSourceFromDescriptor(selected_src);
    source_ptr->open();
    source_ptr->set_frequency(frequency);
    source_ptr->set_samplerate(samplerate);
    source_ptr->set_settings(parameters);

    // Attempt to start the source
    try
    {
        source_ptr->start();
    }
    catch (std::exception &e)
    {
        logger->error("Fatal error starting device : " + std::string(e.what()));
        return 1;
    }

    // Setup file sink
    std::shared_ptr<dsp::FileSinkBlock> file_sink = std::make_shared<dsp::FileSinkBlock>(source_ptr->output_stream);

    int ziq_bit_depth = 8;
    if (parameters.contains("ziq_depth"))
        ziq_bit_depth = parameters["ziq_depth"].get<int>();

    if (parameters["baseband_format"].get<std::string>() == "ziq")
        logger->info("Using ZIQ Depth {:d}", ziq_bit_depth);

    if (parameters.contains("baseband_format"))
    {
        file_sink->set_output_sample_type(dsp::basebandTypeFromString(parameters["baseband_format"].get<std::string>()));
    }
    else
    {
        logger->error("baseband_format flag is required!");
        return 1;
    }

    file_sink->start();
    file_sink->start_recording(output_file, samplerate, ziq_bit_depth);

    // Attach signal
    signal(SIGINT, sig_handler_rec);

    // Now, we wait
    uint64_t start_time = time(0);
    while (1)
    {
        uint64_t elapsed_time = time(0) - start_time;

        if (timeout > 0)
        {
            if (elapsed_time >= timeout)
            {
                logger->warn("Timeout is over! ({:d}s >= {:d}s) Stopping.", elapsed_time, timeout);
                break;
            }
        }

        if (rec_should_exit)
        {
            logger->warn("SIGINT Received. Stopping.");
            break;
        }

        if (int(elapsed_time) % 2 == 0)
        {
            if (parameters["baseband_format"].get<std::string>() == "ziq")
                logger->info("Wrote {:d} MB, raw {:d} MB", int(file_sink->get_written() / 1e6), int(file_sink->get_written_raw() / 1e6));
            else
                logger->info("Wrote {:d} MB", int(file_sink->get_written() / 1e6));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop recording
    file_sink->stop_recording();

    // Stop cleanly
    source_ptr->stop();
    file_sink->stop();

    return 0;
}