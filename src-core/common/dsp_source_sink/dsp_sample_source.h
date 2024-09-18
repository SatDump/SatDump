#pragma once

#include "common/dsp/complex.h"
#include "nlohmann/json.hpp"
#include "common/dsp/buffer.h"
#include <functional>
#include <memory>
#include "nlohmann/json_utils.h"
#include "format_notated.h"
#include "core/exception.h"

namespace dsp
{
    struct SourceDescriptor
    {
        std::string source_type;
        std::string name;
        std::string unique_id;

        bool remote_ok = true;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(SourceDescriptor,
                                       source_type, name, unique_id)
    };

    class DSPSampleSource
    {
    public:
        std::shared_ptr<dsp::stream<complex_t>> output_stream;
        nlohmann::json d_settings;
        uint64_t d_frequency;
        std::string d_sdr_id;

    protected:
        inline int calculate_buffer_size_from_samplerate(int samplerate, int buffer_per_sec = 60, int blocksize = 512)
        {
            return std::min<int>(ceil((double)samplerate / double(buffer_per_sec * blocksize)) * blocksize, dsp::STREAM_BUFFER_SIZE);
        }

    public:
        virtual void open() = 0;                                                              // Open the device, source, etc, but don't start the stream yet
        virtual void start() { output_stream = std::make_shared<dsp::stream<complex_t>>(); }; // Start streaming samples
        virtual void stop() = 0;                                                              // Stop streaming samples
        virtual void close() = 0;                                                             // Close the device, source etc

        virtual void drawControlUI() = 0; // Draw the source control UI, eg, gain settings etc

        virtual void set_settings(nlohmann::json settings) { d_settings = settings; }
        virtual nlohmann::json get_settings() { return d_settings; }

        virtual void set_frequency(uint64_t frequency) { d_frequency = frequency; }
        virtual uint64_t get_frequency() { return d_frequency; }

        virtual void set_samplerate(uint64_t samplerate) = 0; // This should set the samplerate of the device
        virtual uint64_t get_samplerate() = 0;                // This should return the current samplerate of the device

        DSPSampleSource(SourceDescriptor source)
        {
            d_sdr_id = source.unique_id;
        }

    public:
        static std::string getID();
        static std::shared_ptr<DSPSampleSource> getInstance(SourceDescriptor source);
        static std::vector<SourceDescriptor> getAvailableSources();
    };

    struct RegisteredSource
    {
        std::function<std::shared_ptr<DSPSampleSource>(SourceDescriptor)> getInstance;
        std::function<std::vector<SourceDescriptor>()> getSources;
    };

    struct RegisterDSPSampleSourcesEvent
    {
        std::map<std::string, RegisteredSource> &dsp_sources_registry;
    };

    extern std::map<std::string, RegisteredSource> dsp_sources_registry;
    void registerAllSources();
    std::vector<SourceDescriptor> getAllAvailableSources(bool remote = false);
    std::shared_ptr<DSPSampleSource> getSourceFromDescriptor(SourceDescriptor descriptor);
}