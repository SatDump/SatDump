#pragma once

#include "common/dsp/block.h"
#include "libs/predict/predict.h"

/*
Costas loop implementation
with order 2, 4 and 8 support
*/
namespace dsp
{
    class DopplerCorrectBlock : public Block<complex_t, complex_t>
    {
    private:
        const double d_samplerate;
        const float d_alpha;                 // Rate at which we catch up with target_freq
        const double d_signal_frequency = 0; // Actual OTA frequency
        float targ_freq = 0;                 // Target frequency, in radians per sample
        float curr_phase = 0, curr_freq = 0; // Current phase & freq

        // Predict
        predict_orbital_elements_t *satellite_object = nullptr;
        predict_position satellite_orbit;
        predict_observer_t *observer_station;
        predict_observation observation_pos;

        void work();

    public:
        DopplerCorrectBlock(std::shared_ptr<dsp::stream<complex_t>> input,
                            double samplerate, float alpha, double signal_frequency, int norad,
                            double qth_lon, double qth_lat, double qth_alt);
        ~DopplerCorrectBlock();

        float getFreq() { return curr_freq; }

        double start_time = -1;
    };
}