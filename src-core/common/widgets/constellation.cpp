#include "constellation.h"
#include <cstring>
#include "module.h"

namespace widgets
{
    ConstellationViewer::ConstellationViewer(float hscale, float vscale, int constellation_size) : d_hscale(hscale), d_vscale(vscale), d_constellation_size(constellation_size)
    {
    }

    ConstellationViewer::~ConstellationViewer()
    {
    }

    void ConstellationViewer::pushComplex(std::complex<float> *buffer, int size)
    {
        int to_copy = std::min<int>(CONST_SIZE, size);
        int to_move = size >= CONST_SIZE ? 0 : CONST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float[size], sample_buffer_complex_float, to_move * sizeof(std::complex<float>));

        std::memcpy(sample_buffer_complex_float, buffer, to_copy * sizeof(std::complex<float>));
    }

    void ConstellationViewer::pushFloatAndGaussian(float *buffer, int size)
    {
        int to_copy = std::min<int>(CONST_SIZE, size);
        int to_move = size >= CONST_SIZE ? 0 : CONST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float[size], sample_buffer_complex_float, to_move * sizeof(std::complex<float>));

        for (int i = 0; i < to_copy; i++)
            sample_buffer_complex_float[i] = std::complex<float>(buffer[i], rng.gasdev());
    }

    void ConstellationViewer::draw()
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                 ImVec2(ImGui::GetCursorScreenPos().x + d_constellation_size * ui_scale, ImGui::GetCursorScreenPos().y + d_constellation_size * ui_scale),
                                 ImColor::HSV(0, 0, 0));

        for (int i = 0; i < CONST_SIZE; i++)
        {
            draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)((d_constellation_size / 2) * ui_scale + sample_buffer_complex_float[i].real() * (d_constellation_size / 2) * d_hscale * ui_scale) % int(d_constellation_size * ui_scale),
                                              ImGui::GetCursorScreenPos().y + (int)((d_constellation_size / 2) * ui_scale + sample_buffer_complex_float[i].imag() * (d_constellation_size / 2) * d_vscale * ui_scale) % int(d_constellation_size * ui_scale)),
                                       2 * ui_scale * (d_constellation_size / 200.0f),
                                       ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
        }

        ImGui::Dummy(ImVec2(d_constellation_size * ui_scale + 3, d_constellation_size * ui_scale + 3));
    }
}