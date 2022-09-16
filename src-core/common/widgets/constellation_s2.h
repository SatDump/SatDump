#pragma once

#include "imgui/imgui.h"
#include "common/dsp/complex.h"

namespace widgets
{
#define CONST_SIZE 2048

    class ConstellationViewerDVBS2
    {
    private:
        complex_t sample_buffer_complex_float_plheader[CONST_SIZE / 4];
        complex_t sample_buffer_complex_float_slots[CONST_SIZE];
        complex_t sample_buffer_complex_float_pilots[CONST_SIZE];
        const float d_hscale, d_vscale;
        const int d_constellation_size;
        bool has_pilots = false;

    public:
        ConstellationViewerDVBS2(float hscale = 1, float vscale = 1, int constellation_size = 200);
        ~ConstellationViewerDVBS2();
        void pushComplexPL(complex_t *buffer, int size);
        void pushComplexSlots(complex_t *buffer, int size);
        void pushComplexPilots(complex_t *buffer, int size);
        void draw();
    };
}