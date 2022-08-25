#include "logger.h"
#include "init.h"
#include "common/dsp_sample_source/dsp_sample_source.h"

int main(int argc, char *argv[])
{
    initLogger();

    // We don't wanna spam with init this time around
    logger->set_level(spdlog::level::level_enum::off);
    satdump::initSatdump();
    logger->set_level(spdlog::level::level_enum::trace);

    dsp::registerAllSources();

    std::vector<dsp::SourceDescriptor> sources = dsp::getAllAvailableSources();

    logger->info("Found devices :");
    for (dsp::SourceDescriptor src : sources)
        logger->info("- " + src.name);
}
