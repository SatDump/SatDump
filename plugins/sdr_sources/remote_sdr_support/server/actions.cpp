#include "actions.h"
#include "main.h"
#include "streaming.h"

// The source calls below (start, stop, close...) may throw, for instance when
// the device cannot be claimed because another program is using it. Scoped
// locks make sure the mutexes are always released, otherwise every later
// packet (and the GUI thread) would block forever on source_mtx.

void action_sourceStop()
{
    std::lock_guard<std::mutex> lock(source_mtx);
    if (source_is_started)
    {
        source_should_stream = false;
        source_is_started = false;
        std::lock_guard<std::mutex> lock_stream(source_stream_mtx);
        current_sample_source->stop();
        current_sample_source->output_stream->stopReader();
        current_sample_source->output_stream->stopWriter();
        logger->info("Source stopped!");
    }
}

void action_sourceStart()
{
    std::lock_guard<std::mutex> lock(source_mtx);
    std::lock_guard<std::mutex> lock_stream(source_stream_mtx);
    if (!source_is_started && source_is_open && current_sample_source)
    {
        current_sample_source->start();
        source_should_stream = true;
        source_is_started = true;
        logger->info("Source started!");
    }
}

void action_sourceClose()
{
    std::lock_guard<std::mutex> lock(source_mtx);
    if (source_is_open && current_sample_source)
    {
        source_is_open = false;
        current_sample_source->close();
        current_sample_source.reset();
        logger->info("Source closed!");
    }
}
