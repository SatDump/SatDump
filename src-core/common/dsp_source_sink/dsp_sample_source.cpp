#include "dsp_sample_source.h"
#include "core/plugin.h"

#include "file_source.h"

#include "logger.h"

namespace dsp
{
    std::map<std::string, RegisteredSource> dsp_sources_registry;

    std::vector<SourceDescriptor> getAllAvailableSources(bool remote)
    {
        std::vector<SourceDescriptor> all_sources;

        for (std::pair<std::string, RegisteredSource> source : dsp_sources_registry)
        {
            if (remote)
                if (source.first == "remote")
                    continue;
            try
            {
                std::vector<SourceDescriptor> devices = source.second.getSources();
                all_sources.insert(all_sources.end(), devices.begin(), devices.end());
            }
            catch (std::exception &e)
            {
                logger->error("Error trying to list " + source.first + " sources! %s", e.what());
            }
        }

        return all_sources;
    }

    std::shared_ptr<DSPSampleSource> getSourceFromDescriptor(SourceDescriptor descriptor)
    {
        for (std::pair<std::string, RegisteredSource> source : dsp_sources_registry)
            if (descriptor.source_type == source.first)
                return source.second.getInstance(descriptor);
        throw satdump_exception("Could not find a handler for device " + descriptor.name);
    }

    void registerAllSources()
    {
        dsp_sources_registry.insert({FileSource::getID(), {FileSource::getInstance, FileSource::getAvailableSources}});

        // Plugin Sources
        satdump::eventBus->fire_event<RegisterDSPSampleSourcesEvent>({dsp_sources_registry});
    }
}