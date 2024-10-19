#include "constellation.h"
#include <cstring>
#include "core/style.h"
#include "common/dsp/block.h"

namespace widgets
{
    ConstellationViewer::ConstellationViewer(float hscale, float vscale, int constellation_size) : d_constellation_size(constellation_size), d_hscale(hscale), d_vscale(vscale)
    {
    }

    ConstellationViewer::~ConstellationViewer()
    {
    }

    void ConstellationViewer::pushComplexScaled(complex_t *buffer, int size, float scale)
    {
        int to_copy = std::min<int>(CONST_SIZE, size);
        int to_move = size >= CONST_SIZE ? 0 : CONST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float[size], sample_buffer_complex_float, to_move * sizeof(complex_t));

        for (int i = 0; i < to_copy; i++)
            sample_buffer_complex_float[i] = buffer[i] * scale;
    }

    void ConstellationViewer::pushComplex(complex_t *buffer, int size)
    {
        int to_copy = std::min<int>(CONST_SIZE, size);
        int to_move = size >= CONST_SIZE ? 0 : CONST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float[size], sample_buffer_complex_float, to_move * sizeof(complex_t));

        std::memcpy(sample_buffer_complex_float, buffer, to_copy * sizeof(complex_t));
    }

    void ConstellationViewer::pushFloatAndGaussian(float *buffer, int size)
    {
        int to_copy = std::min<int>(CONST_SIZE, size);
        int to_move = size >= CONST_SIZE ? 0 : CONST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float[size], sample_buffer_complex_float, to_move * sizeof(complex_t));

        for (int i = 0; i < to_copy; i++)
            sample_buffer_complex_float[i] = complex_t(buffer[i], rng.gasdev());
    }

    void ConstellationViewer::pushSofttAndGaussian(int8_t *buffer, float scale, int size)
    {
        int to_copy = std::min<int>(CONST_SIZE, size);
        int to_move = size >= CONST_SIZE ? 0 : CONST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float[size], sample_buffer_complex_float, to_move * sizeof(complex_t));

        for (int i = 0; i < to_copy; i++)
            sample_buffer_complex_float[i] = complex_t(buffer[i] / scale, rng.gasdev());
    }

    void ConstellationViewer::draw()
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImVec2 rect_min = ImGui::GetCursorScreenPos();
        ImVec2 rect_max = { rect_min.x + d_constellation_size * ui_scale, rect_min.y + d_constellation_size * ui_scale };
        draw_list->AddRectFilled(rect_min, rect_max, style::theme.widget_bg);
        draw_list->PushClipRect(rect_min, rect_max);

        for (int i = 0; i < CONST_SIZE; i++)
        {
            draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + ((d_constellation_size / 2) * ui_scale + sample_buffer_complex_float[i].real * (d_constellation_size / 2) * d_hscale * ui_scale),
                                              ImGui::GetCursorScreenPos().y + ((d_constellation_size / 2) * ui_scale + sample_buffer_complex_float[i].imag * (d_constellation_size / 2) * d_vscale * ui_scale)),
                                       2 * ui_scale * (d_constellation_size / 200.0f),
                                       style::theme.constellation);
        }

        draw_list->PopClipRect();
        ImGui::Dummy(ImVec2(d_constellation_size * ui_scale + 3, d_constellation_size * ui_scale + 3));
    }
}