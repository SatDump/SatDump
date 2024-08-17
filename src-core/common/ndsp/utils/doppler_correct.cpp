#include "doppler_correct.h"
#include "common/dsp/block.h"

#include "common/geodetic/geodetic_coordinates.h"
#include "common/tracking/tle.h"

#define SPEED_OF_LIGHT_M_S 299792458.0

namespace ndsp
{
    DopplerCorrect::DopplerCorrect()
        : ndsp::Block("doppler_correct", {{sizeof(complex_t)}}, {{sizeof(complex_t)}})
    {
    }

    DopplerCorrect::~DopplerCorrect()
    {
        predict_destroy_observer(observer_station);
        predict_destroy_orbital_elements(satellite_object);
    }

    void DopplerCorrect::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<complex_t>(outputs[0], 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        ndsp::Block::start();
    }

    void DopplerCorrect::set_params(nlohmann::json p)
    {
        samplerate = d_samplerate;
        alpha = d_apha;
        signal_frequency = d_signal_frequency;

        // Init sat object
        auto tle = satdump::general_tle_registry.get_from_norad(d_norad).value();
        satellite_object = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());

        // Init obs object
        observer_station = predict_create_observer("Main", d_qth_lat * DEG_TO_RAD, d_qth_lon * DEG_TO_RAD, d_qth_alt);

        if (observer_station == nullptr || satellite_object == nullptr)
            throw std::runtime_error("Couldn't init libpredict objects!");
    }

    inline double getTime()
    {
        auto time = std::chrono::system_clock::now();
        auto since_epoch = time.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
        return millis.count() / 1e3;
    }

    void DopplerCorrect::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<complex_t> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<complex_t> *)outputs[0]->write_buf();

            int nsamples = rbuf->cnt;

            for (int i = 0; i < rbuf->cnt; i++)
            {
                // Mix input & VCO
                wbuf->dat[i] = rbuf->dat[i] * complex_t(cosf(-curr_phase), sinf(-curr_phase));

                // Increment phase
                curr_phase += curr_freq;

                // Wrap phase
                while (curr_phase > (2 * M_PI))
                    curr_phase -= 2 * M_PI;
                while (curr_phase < (-2 * M_PI))
                    curr_phase += 2 * M_PI;

                // Slowly ramp up freq toward target freq
                curr_freq = curr_freq * (1.0 - alpha) + targ_freq * alpha;
            }

            wbuf->cnt = rbuf->cnt;

            outputs[0]->write();
            inputs[0]->flush();

            // Get current time.
            double curr_time;
            if (start_time != -1)
            { // Either from a baseband, from samplerate
                start_time += (double)nsamples / samplerate;
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
            double doppler_shift = (observation_pos.range_rate * 1000.0 / SPEED_OF_LIGHT_M_S) * signal_frequency;

            // printf("%f %f\n", doppler_shift, observation_pos.elevation * RAD_TO_DEG);

            // Convert to a phase delta
            targ_freq = dsp::hz_to_rad(-doppler_shift, samplerate);
        }
    }
}
