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
    struct SinkDescriptor
    {
        std::string sink_type;
        std::string name;
        std::string unique_id;
    };

    class DSPSampleSink
    {
    public:
        std::shared_ptr<dsp::stream<complex_t>> input_stream;
        nlohmann::json d_settings;
        uint64_t d_frequency;
        std::string d_sdr_id;

    public:
        virtual void open() = 0;                                                                      // Open the device, sink, etc, but don't start the stream yet
        virtual void start(std::shared_ptr<dsp::stream<complex_t>> stream) { input_stream = stream; } // Start streaming samples
        virtual void stop()                                                                           // Stop streaming samples
        {
            input_stream->stopReader();
            input_stream->stopWriter();
        };
        virtual void close() = 0; // Close the device, sink etc

        virtual void drawControlUI() = 0; // Draw the sink control UI, eg, gain settings etc

        virtual void set_settings(nlohmann::json settings) { d_settings = settings; }
        virtual nlohmann::json get_settings() { return d_settings; }

        virtual void set_frequency(uint64_t frequency) { d_frequency = frequency; }
        virtual uint64_t get_frequency() { return d_frequency; }

        virtual void set_samplerate(uint64_t samplerate) = 0; // This should set the samplerate of the device
        virtual uint64_t get_samplerate() = 0;                // This should return the current samplerate of the device

        DSPSampleSink(SinkDescriptor source)
        {
            d_sdr_id = source.unique_id;
        }

    public:
        static std::string getID();
        static std::shared_ptr<DSPSampleSink> getInstance(SinkDescriptor source);
        static std::vector<SinkDescriptor> getAvailableSinks();
    };

    struct RegisteredSink
    {
        std::function<std::shared_ptr<DSPSampleSink>(SinkDescriptor)> getInstance;
        std::function<std::vector<SinkDescriptor>()> getSources;
    };

    struct RegisterDSPSampleSinksEvent
    {
        std::map<std::string, RegisteredSink> &dsp_sinks_registry;
    };

    extern std::map<std::string, RegisteredSink> dsp_sinks_registry;
    void registerAllSinks();
    std::vector<SinkDescriptor> getAllAvailableSinks();
    std::shared_ptr<DSPSampleSink> getSinkFromDescriptor(SinkDescriptor descriptor);
}