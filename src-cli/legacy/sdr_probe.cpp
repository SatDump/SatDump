#include "logger.h"
#include "init.h"
#include "sdr_probe.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/dsp_source_sink/dsp_sample_sink.h"

void sdr_probe()
{
    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_ERROR);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    dsp::registerAllSources();
    dsp::registerAllSinks();

    std::vector<dsp::SourceDescriptor> sources = dsp::getAllAvailableSources();
    std::vector<dsp::SinkDescriptor> sinks = dsp::getAllAvailableSinks();

    logger->info("Found devices (sources) :");
    for (dsp::SourceDescriptor src : sources)
    {
        logger->info("- " + src.name);
        logger->debug("    - type : " + src.source_type);
        logger->debug("    - id : %s", src.unique_id.c_str());
    }

    logger->info("Found devices (sinks) :");
    for (dsp::SinkDescriptor src : sinks)
    {
        logger->info("- " + src.name);
        logger->debug("    - type : " + src.sink_type);
        logger->debug("    - id : %s", src.unique_id.c_str());
    }
}
