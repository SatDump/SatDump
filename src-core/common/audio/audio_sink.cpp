#include "audio_sink.h"
#include "core/plugin.h"
#include "logger.h"

namespace audio
{
    std::map<std::string, std::function<std::shared_ptr<AudioSink>()>> sink_registry;

    AudioSink::AudioSink()
    {
    }

    class DummyAudioSink : public AudioSink
    {
    public:
        void set_samplerate(int) {}
        void start() {}
        void stop() {}

        void push_samples(int16_t *, int) {}

    public:
        static std::string getID() { return "dummy"; }
        static std::shared_ptr<AudioSink> getInstance() { return std::make_shared<DummyAudioSink>(); }
    };

    void registerSinks()
    {
        satdump::eventBus->fire_event<RegisterAudioSinkEvent>({sink_registry});
    }

    std::shared_ptr<AudioSink> get_default_sink()
    {
        if (sink_registry.count("portaudio"))
        {
            logger->info("Using PortAudio Sink");
            return sink_registry["portaudio"]();
        }
        else if (sink_registry.count("rtaudio"))
        {
            logger->info("Using RTAudio Sink");
            return sink_registry["rtaudio"]();
        }
        else
            return std::make_shared<DummyAudioSink>();
    }

    bool has_sink()
    {
        return sink_registry.size() > 0;
    }

    // https://github.com/cpuimage/resampler, just wanted something super basic
    int AudioSink::resample_s16(const int16_t *input, int16_t *output, int inSampleRate, int outSampleRate, int inputSize, int channels)
    {
        if (input == NULL)
            return 0;
        uint64_t outputSize = (uint64_t)(inputSize * (double)outSampleRate / (double)inSampleRate);
        outputSize -= outputSize % channels;
        if (output == NULL)
            return outputSize;
        double stepDist = ((double)inSampleRate / (double)outSampleRate);
        const uint64_t fixedFraction = (1LL << 32);
        const double normFixed = (1.0 / (1LL << 32));
        uint64_t step = ((uint64_t)(stepDist * fixedFraction + 0.5));
        uint64_t curOffset = 0;
        for (uint32_t i = 0; i < outputSize; i += 1)
        {
            for (int c = 0; c < channels; c += 1)
            {
                *output++ = (int16_t)(input[c] + (input[c + channels] - input[c]) * ((double)(curOffset >> 32) + ((curOffset & (fixedFraction - 1)) * normFixed)));
            }
            curOffset += step;
            input += (curOffset >> 32) * channels;
            curOffset &= (fixedFraction - 1);
        }
        return outputSize;
    }
}