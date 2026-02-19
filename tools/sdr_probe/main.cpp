#include "logger.h"
#include "init.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/dsp_source_sink/dsp_sample_sink.h"

int main(/*int argc, char *argv[]*/)
{
    initLogger();

    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_OFF);
    satdump::initSatDump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    dsp::registerAllSources();
    dsp::registerAllSinks();

    std::vector<dsp::SourceDescriptor> sources = dsp::getAllAvailableSources();
    std::vector<dsp::SinkDescriptor> sinks = dsp::getAllAvailableSinks();

    logger->info("Found devices (sources) :");
    for (dsp::SourceDescriptor src : sources)
        logger->info("- " + src.name);

    logger->info("Found devices (sinks) :");
    for (dsp::SinkDescriptor src : sinks)
        logger->info("- " + src.name);

    satdump::exitSatDump();
}
