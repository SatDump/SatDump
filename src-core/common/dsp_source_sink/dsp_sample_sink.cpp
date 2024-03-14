#include "dsp_sample_sink.h"
#include "core/plugin.h"

#include "file_source.h"

namespace dsp
{
    std::map<std::string, RegisteredSink> dsp_sinks_registry;

    std::vector<SinkDescriptor> getAllAvailableSinks()
    {
        std::vector<SinkDescriptor> all_sinks;

        for (std::pair<std::string, RegisteredSink> source : dsp_sinks_registry)
        {
            std::vector<SinkDescriptor> devices = source.second.getSources();
            all_sinks.insert(all_sinks.end(), devices.begin(), devices.end());
        }

        return all_sinks;
    }

    std::shared_ptr<DSPSampleSink> getSinkFromDescriptor(SinkDescriptor descriptor)
    {
        for (std::pair<std::string, RegisteredSink> sink : dsp_sinks_registry)
            if (descriptor.sink_type == sink.first)
                return sink.second.getInstance(descriptor);
        throw satdump_exception("Could not find a handler for device " + descriptor.name);
    }

    void registerAllSinks()
    {
        // dsp_sinks_registry.insert({FileSource::getID(), {FileSource::getInstance, FileSource::getAvailableSinks}});

        // Plugin Sources
        satdump::eventBus->fire_event<RegisterDSPSampleSinksEvent>({dsp_sinks_registry});
    }
}