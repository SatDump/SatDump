#include "actions.h"
#include "main.h"
#include "streaming.h"

void action_sourceStop()
{
    source_mtx.lock();
    if (source_is_started)
    {
        source_should_stream = false;
        source_stream_mtx.lock();
        current_sample_source->stop();
        current_sample_source->output_stream->stopReader();
        current_sample_source->output_stream->stopWriter();
        source_stream_mtx.unlock();
        logger->info("Source stopped!");
        source_is_started = false;
    }
    source_mtx.unlock();
}

void action_sourceStart()
{
    source_mtx.lock();
    source_stream_mtx.lock();
    if (!source_is_started && current_sample_source)
    {
        current_sample_source->start();
        source_should_stream = true;
        source_is_started = true;
        logger->info("Source started!");
    }
    source_stream_mtx.unlock();
    source_mtx.unlock();
}

void action_sourceClose()
{
    source_mtx.lock();
    if (source_is_open && current_sample_source)
    {
        current_sample_source->close();
        current_sample_source.reset();
        source_is_open = false;
        logger->info("Source closed!");
    }
    source_mtx.unlock();
}
