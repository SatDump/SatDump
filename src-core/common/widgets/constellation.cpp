#include "constellation.h"
#include <cstring>
#include "module.h"

namespace widgets
{
    ConstellationViewer::ConstellationViewer(float hscale, float vscale) : d_hscale(hscale), d_vscale(vscale)
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

    void ConstellationViewer::draw()
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                 ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                 ImColor::HSV(0, 0, 0));

        for (int i = 0; i < CONST_SIZE; i++)
        {
            draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + sample_buffer_complex_float[i].real() * 100 * d_hscale * ui_scale) % int(200 * ui_scale),
                                              ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + sample_buffer_complex_float[i].imag() * 100 * d_vscale * ui_scale) % int(200 * ui_scale)),
                                       2 * ui_scale,
                                       ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
        }

        ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
    }
}