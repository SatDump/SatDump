#include "sdr_probe.h"
#include "common/dsp_source_sink/dsp_sample_sink.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "init.h"
#include "logger.h"

void sdr_probe()
{
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
