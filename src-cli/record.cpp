#include "live.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/dsp/resamp/smart_resampler.h"
#include <signal.h>
#include "logger.h"
#include "common/cli_utils.h"
#include "common/dsp/io/file_sink.h"
#include "init.h"
#include "common/dsp/path/splitter.h"
#include "common/dsp/fft/fft_pan.h"
#include "webserver.h"

// Catch CTRL+C to exit live properly!
bool rec_should_exit = false;
void sig_handler_rec(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
        rec_should_exit = true;
}

int main_record(int argc, char *argv[])
{
    if (argc < 5) // Check overall command
    {
        logger->error("Usage : " + std::string(argv[0]) + " record [output_baseband (without extension!)] [additional options as required]");
        logger->error("Extra options (examples. Any parameter used in sources can be used here) :");
        logger->error(" --samplerate [baseband_samplerate] --baseband_format [cf32/cs32/cs16/cs8/cu8/wav16/ziq] --dc_block --iq_swap");
        logger->error(" --source [airspy/rtlsdr/etc] --gain 20 --bias");
        logger->error("As well as --timeout in seconds");
        logger->error("Sample command :");
        logger->error("./satdump record baseband_name --source airspy --samplerate 6e6 --frequency 1701.3e6 --general_gain 18 --bias --timeout 780");
        return 1;
    }

    // Init SatDump
    satdump::initSatdump();
    completeLoggerInit();

    std::string output_file = argv[2];

    // Parse flags
    nlohmann::json parameters = parse_common_flags(argc - 3, &argv[3], {{"source_id", typeid(std::string)}});

    uint64_t samplerate;
    uint64_t frequency;
    uint64_t timeout;
    std::string handler_id;
    std::string hdl_dev_id;
    double decimation = 1;

    try
    {
        samplerate = parameters["samplerate"].get<uint64_t>();
        frequency = parameters["frequency"].get<uint64_t>();
        timeout = parameters.contains("timeout") ? parameters["timeout"].get<uint64_t>() : 0;
        handler_id = parameters["source"].get<std::string>();
        if (parameters.contains("decimation"))
            decimation = parameters["decimation"].get<int>();
        if (parameters.contains("source_id"))
            hdl_dev_id = parameters["source_id"].get<std::string>();
    }
    catch (std::exception &e)
    {
        logger->error("Error parsing arguments! %s", e.what());
        return 1;
    }

    // Create output dir
    if (std::filesystem::path(output_file).has_parent_path())
        if (!std::filesystem::exists(std::filesystem::path(output_file).parent_path().string()))
            std::filesystem::create_directories(std::filesystem::path(output_file).parent_path().string());

    // Get all sources
    dsp::registerAllSources();
    std::vector<dsp::SourceDescriptor> source_tr = dsp::getAllAvailableSources();
    dsp::SourceDescriptor selected_src;

    // Try to find it and check it's usable
    bool src_found = false;
    for (dsp::SourceDescriptor src : source_tr)
    {
        logger->debug("Device " + src.name);
        if (handler_id == src.source_type)
        {
            if (parameters.contains("source_id"))
            {
                if (hdl_dev_id == src.unique_id)
                {
                    selected_src = src;
                    src_found = true;
                }
            }
            else
            {
                selected_src = src;
                src_found = true;
            }
        }
    }

    if (!src_found)
    {
        logger->error("Could not find a handler for source type : %s!", handler_id.c_str());
        return 1;
    }

    // Init source
    std::shared_ptr<dsp::DSPSampleSource> source_ptr = getSourceFromDescriptor(selected_src);
    source_ptr->open();
    source_ptr->set_frequency(frequency);
    source_ptr->set_samplerate(samplerate);
    source_ptr->set_settings(parameters);

    std::unique_ptr<dsp::SmartResamplerBlock<complex_t>> decim;
    std::unique_ptr<dsp::SplitterBlock> splitter;
    std::unique_ptr<dsp::FFTPanBlock> fft;
    bool webserver_already_set = false;

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

    // Decimation if requested
    if (decimation > 1)
    {
        decim = std::make_unique<dsp::SmartResamplerBlock<complex_t>>(source_ptr->output_stream, 1, decimation);
        decim->start();
        logger->info("Setting up resampler...");
    }

    // Optional FFT
    std::shared_ptr<dsp::stream<complex_t>> final_stream = decimation > 1 ? decim->output_stream : source_ptr->output_stream;

    int fft_size = 0;
    if (parameters.contains("fft_enable"))
    {
        fft_size = parameters.contains("fft_size") ? parameters["fft_size"].get<int>() : 512;
        int fft_rate = parameters.contains("fft_rate") ? parameters["fft_rate"].get<int>() : 30;

        splitter = std::make_unique<dsp::SplitterBlock>(source_ptr->output_stream);
        splitter->add_output("fft");
        splitter->set_enabled("fft", true);
        final_stream = splitter->output_stream;
        fft = std::make_unique<dsp::FFTPanBlock>(splitter->get_output("fft"));
        fft->set_fft_settings(fft_size, samplerate / decimation, fft_rate);
        if (parameters.contains("fft_avgn"))
            fft->avg_num = parameters["fft_avgn"].get<float>();
        splitter->start();
        fft->start();
    }

    // Setup file sink
    std::shared_ptr<dsp::FileSinkBlock> file_sink = std::make_shared<dsp::FileSinkBlock>(final_stream);

    if (parameters.contains("fft_enable"))
    {
        webserver::handle_callback = [&file_sink, &fft, fft_size]()
        {
            nlohmann::json stats;
            stats["written"] = file_sink->get_written();
            stats["written_raw"] = file_sink->get_written_raw();
            for (int i = 0; i < fft_size; i++)
                stats["fft_values"][i] = fft->output_stream->writeBuf[i];
            return stats.dump(4);
        };

        webserver_already_set = true;
    }

    if (!parameters.contains("baseband_format"))
    {
        logger->error("baseband_format flag is required!");
        return 1;
    }

    dsp::BasebandType baseband_type = parameters["baseband_format"].get<std::string>();

#if defined(BUILD_ZIQ) || defined(BUILD_ZIQ2)
    if (parameters.contains("ziq_depth"))
        baseband_type.ziq_depth = parameters["ziq_depth"].get<int>();

    if (parameters["baseband_format"].get<std::string>() == "ziq")
        logger->info("Using ZIQ Depth %d", baseband_type.ziq_depth);
#endif

    file_sink->set_output_sample_type(baseband_type);
    file_sink->start();
    file_sink->start_recording(output_file, samplerate / decimation);

    // If requested, boot up webserver
    if (parameters.contains("http_server"))
    {
        std::string http_addr = parameters["http_server"].get<std::string>();
        if (!webserver_already_set)
            webserver::handle_callback = [&file_sink]()
            {
                nlohmann::json stats;
                stats["written"] = file_sink->get_written();
                stats["written_raw"] = file_sink->get_written_raw();
                return stats.dump(4);
            };
        logger->info("Start webserver on %s", http_addr.c_str());
        webserver::start(http_addr);
    }

    // Attach signal
    signal(SIGINT, sig_handler_rec);
    signal(SIGTERM, sig_handler_rec);

    // Now, we wait
    uint64_t start_time = time(0);
    while (1)
    {
        uint64_t elapsed_time = time(0) - start_time;

        if (timeout > 0)
        {
            if (elapsed_time >= timeout)
            {
                logger->warn("Timeout is over! (%ds >= %ds) Stopping.", elapsed_time, timeout);
                break;
            }
        }

        if (rec_should_exit)
        {
            logger->warn("Signal Received. Stopping.");
            break;
        }

        if (int(elapsed_time) % 2 == 0)
        {
            if (parameters["baseband_format"].get<std::string>() == "ziq")
                logger->info("Wrote %d MB, raw %d MB", int(file_sink->get_written() / 1e6), int(file_sink->get_written_raw() / 1e6));
            else
                logger->info("Wrote %d MB", int(file_sink->get_written() / 1e6));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop recording
    file_sink->stop_recording();

    // Stop cleanly
    source_ptr->stop();
    if (parameters.contains("fft_enable"))
    {
        splitter->stop();
        fft->stop();
    }
    file_sink->stop();

    if (parameters.contains("http_server"))
        webserver::stop();

    return 0;
}
