#include "common/audio/audio_sink.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
    // Records which factory produced it
    class TaggedSink : public audio::AudioSink
    {
    public:
        std::string tag;
        TaggedSink(std::string t) : tag(t) {}
        void set_samplerate(int) {}
        void start() {}
        void stop() {}
        void push_samples(int16_t *, int) {}
    };

    // Temporarily swaps the global sink registry
    struct RegistryGuard
    {
        std::map<std::string, std::function<std::shared_ptr<audio::AudioSink>()>> saved;
        RegistryGuard() : saved(audio::sink_registry) { audio::sink_registry.clear(); }
        ~RegistryGuard() { audio::sink_registry = saved; }
    };
} // namespace

TEST_CASE("audio::AudioSink::resample_s16 at equal rates copies the input", "[common][audio]")
{
    // One extra sample: the interpolator reads input[i + channels] for the last output
    std::vector<int16_t> in = {1, -2, 300, -400, 5000, -6000, 32767, -32768, 0};
    std::vector<int16_t> out(8, 0x5555);

    int n = audio::AudioSink::resample_s16(in.data(), out.data(), 48000, 48000, 8, 1);

    REQUIRE(n == 8);
    CHECK(out == std::vector<int16_t>(in.begin(), in.begin() + 8));
}

TEST_CASE("audio::AudioSink::resample_s16 decimates by two", "[common][audio]")
{
    std::vector<int16_t> in = {0, 10, 20, 30, 40, 50, 60, 70};
    std::vector<int16_t> out(4);

    int n = audio::AudioSink::resample_s16(in.data(), out.data(), 48000, 24000, 8, 1);

    REQUIRE(n == 4);
    CHECK(out == std::vector<int16_t>{0, 20, 40, 60});
}

TEST_CASE("audio::AudioSink::resample_s16 interpolates when upsampling", "[common][audio]")
{
    // Last value repeated as padding for the read past the end
    std::vector<int16_t> in = {0, 100, 200, 300, 300};
    std::vector<int16_t> out(8);

    int n = audio::AudioSink::resample_s16(in.data(), out.data(), 24000, 48000, 4, 1);

    REQUIRE(n == 8);
    CHECK(out == std::vector<int16_t>{0, 50, 100, 150, 200, 250, 300, 300});
}

TEST_CASE("audio::AudioSink::resample_s16 output size computation", "[common][audio]")
{
    int16_t dummy = 0;
    CHECK(audio::AudioSink::resample_s16(nullptr, nullptr, 48000, 24000, 100, 1) == 0);
    CHECK(audio::AudioSink::resample_s16(&dummy, nullptr, 48000, 44100, 480, 1) == 441);
    CHECK(audio::AudioSink::resample_s16(&dummy, nullptr, 8000, 48000, 100, 1) == 600);
    // Rounded down to a whole number of channel frames
    CHECK(audio::AudioSink::resample_s16(&dummy, nullptr, 48000, 24000, 102, 2) == 50);
    CHECK(audio::AudioSink::resample_s16(&dummy, nullptr, 48000, 48000, 0, 1) == 0);
}

TEST_CASE("audio::get_default_sink falls back to a dummy sink", "[common][audio]")
{
    RegistryGuard guard;

    CHECK_FALSE(audio::has_sink());
    auto sink = audio::get_default_sink();
    REQUIRE(sink != nullptr);
    CHECK(dynamic_cast<TaggedSink *>(sink.get()) == nullptr);

    // The dummy sink accepts everything without side effects
    int16_t samples[4] = {1, 2, 3, 4};
    sink->set_samplerate(48000);
    sink->start();
    sink->push_samples(samples, 4);
    sink->stop();
}

TEST_CASE("audio::get_default_sink prefers portaudio over rtaudio", "[common][audio]")
{
    RegistryGuard guard;

    audio::sink_registry["rtaudio"] = []() { return std::make_shared<TaggedSink>("rtaudio"); };
    CHECK(audio::has_sink());
    auto sink = std::dynamic_pointer_cast<TaggedSink>(audio::get_default_sink());
    REQUIRE(sink != nullptr);
    CHECK(sink->tag == "rtaudio");

    audio::sink_registry["portaudio"] = []() { return std::make_shared<TaggedSink>("portaudio"); };
    sink = std::dynamic_pointer_cast<TaggedSink>(audio::get_default_sink());
    REQUIRE(sink != nullptr);
    CHECK(sink->tag == "portaudio");

    // Unknown sink names are not picked as default
    audio::sink_registry.clear();
    audio::sink_registry["other"] = []() { return std::make_shared<TaggedSink>("other"); };
    CHECK(audio::has_sink());
    CHECK(std::dynamic_pointer_cast<TaggedSink>(audio::get_default_sink()) == nullptr);
}
