#include "core/plugin.h"
#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "dsp/portaudio_sink.h"
#include "dsp/portaudio_source.h"
#include "logger.h"
#include "portaudio_sink.h"

class PortAudioAudioSupport : public satdump::Plugin
{
public:
    std::string getID() { return "rtaudio_audio_sink"; }

    void init()
    {
        satdump::eventBus->register_handler<audio::RegisterAudioSinkEvent>(registerSinks);

        satdump::eventBus->register_handler<satdump::ndsp::RegisterNodesEvent>(registerNodes);
    }

    static void registerSinks(const audio::RegisterAudioSinkEvent &evt) { evt.sink_registry.emplace(PortAudioSink::getID(), PortAudioSink::getInstance); }

    static void registerNodes(const satdump::ndsp::RegisterNodesEvent &evt)
    {
        evt.r.insert(
            {"portaudio_sink_f",
             {"Audio/PortAudio Sink F", [](const satdump::ndsp::Flowgraph *f) { return std::make_shared<satdump::ndsp::NodeInternal>(f, std::make_shared<satdump::ndsp::PortAudioSinkBlock>()); }}});
        evt.r.insert({"portaudio_source_f",
                      {"Audio/PortAudio Source F",
                       [](const satdump::ndsp::Flowgraph *f) { return std::make_shared<satdump::ndsp::NodeInternal>(f, std::make_shared<satdump::ndsp::PortAudioSourceBlock>()); }}});
    }
};

PLUGIN_LOADER(PortAudioAudioSupport)