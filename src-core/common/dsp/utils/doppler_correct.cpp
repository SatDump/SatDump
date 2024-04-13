#include "doppler_correct.h"

#include "common/geodetic/geodetic_coordinates.h"
#include "common/tracking/tle.h"

#define SPEED_OF_LIGHT_M_S 299792458.0

namespace dsp
{
    DopplerCorrectBlock::DopplerCorrectBlock(std::shared_ptr<dsp::stream<complex_t>> input,
                                             double samplerate, float alpha, double signal_frequency, int norad,
                                             double qth_lon, double qth_lat, double qth_alt)
        : Block(input), d_samplerate(samplerate), d_alpha(alpha), d_signal_frequency(signal_frequency)
    {
        // Init sat object
        auto tle = satdump::general_tle_registry.get_from_norad(norad).value();
        satellite_object = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());

        // Init obs object
        observer_station = predict_create_observer("Main", qth_lat * DEG_TO_RAD, qth_lon * DEG_TO_RAD, qth_alt);

        if (observer_station == nullptr || satellite_object == nullptr)
            throw std::runtime_error("Couldn't init libpredict objects!");
    }

    DopplerCorrectBlock::~DopplerCorrectBlock()
    {
        predict_destroy_observer(observer_station);
        predict_destroy_orbital_elements(satellite_object);
    }

    inline double getTime()
    {
        auto time = std::chrono::system_clock::now();
        auto since_epoch = time.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
        return millis.count() / 1e3;
    }

    void DopplerCorrectBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            // Mix input & VCO
            output_stream->writeBuf[i] = input_stream->readBuf[i] * complex_t(cosf(-curr_phase), sinf(-curr_phase));

            // Increment phase
            curr_phase += curr_freq;

            // Wrap phase
            while (curr_phase > (2 * M_PI))
                curr_phase -= 2 * M_PI;
            while (curr_phase < (-2 * M_PI))
                curr_phase += 2 * M_PI;

            // Slowly ramp up freq toward target freq
            curr_freq = curr_freq * (1.0 - d_alpha) + targ_freq * d_alpha;
        }

        input_stream->flush();
        output_stream->swap(nsamples);

        // Get current time.
        double curr_time;
        if (start_time != -1)
        { // Either from a baseband, from samplerate
            start_time += (double)nsamples / d_samplerate;
            curr_time = start_time;
        }
        else
        { // Or the actual time if we're running live
            curr_time = getTime();
        }

        // Get current satellite ephemeris
        predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(curr_time));
        predict_observe_orbit(observer_station, &satellite_orbit, &observation_pos);

        // Calculate doppler shift at the current frequency
        double doppler_shift = (observation_pos.range_rate * 1000.0 / SPEED_OF_LIGHT_M_S) * d_signal_frequency;

        // printf("%f %f\n", doppler_shift, observation_pos.elevation * RAD_TO_DEG);

        // Convert to a phase delta
        targ_freq = hz_to_rad(-doppler_shift, d_samplerate);
    }
}