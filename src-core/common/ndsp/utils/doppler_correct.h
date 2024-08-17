#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

#include "libs/predict/predict.h"

namespace ndsp
{
    class DopplerCorrect : public ndsp::Block
    {
    private:
        void work();

        double samplerate;
        float alpha;                         // Rate at which we catch up with target_freq
        double signal_frequency = 0;         // Actual OTA frequency
        float targ_freq = 0;                 // Target frequency, in radians per sample
        float curr_phase = 0, curr_freq = 0; // Current phase & freq

        // Predict
        predict_orbital_elements_t *satellite_object = nullptr;
        predict_position satellite_orbit;
        predict_observer_t *observer_station;
        predict_observation observation_pos;

        double start_time = -1;

    public:
        int d_norad = -1;
        double d_qth_lon = 0;
        double d_qth_lat = 0;
        double d_qth_alt = 0;
        double d_start_time = -1;
        double d_samplerate = -1;
        double d_apha = -1;
        double d_signal_frequency = -1;

    public:
        DopplerCorrect();
        ~DopplerCorrect();

        void set_params(nlohmann::json p = {});
        void start();
    };
}