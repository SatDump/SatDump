#include "constellation_s2.h"
#include <cstring>
#include <algorithm>
#include "core/style.h"
#include "common/dsp/block.h"

namespace widgets
{
    ConstellationViewerDVBS2::ConstellationViewerDVBS2(float hscale, float vscale, int constellation_size) : d_hscale(hscale), d_vscale(vscale), d_constellation_size(constellation_size)
    {
    }

    ConstellationViewerDVBS2::~ConstellationViewerDVBS2()
    {
    }

    void ConstellationViewerDVBS2::pushComplexPL(complex_t *buffer, int size)
    {
        int to_copy = std::min<int>(CONST_SIZE, size);
        int to_move = size >= CONST_SIZE ? 0 : CONST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float_plheader[size], sample_buffer_complex_float_plheader, to_move * sizeof(complex_t));

        std::memcpy(sample_buffer_complex_float_plheader, buffer, to_copy * sizeof(complex_t));
    }

    void ConstellationViewerDVBS2::pushComplexSlots(complex_t *buffer, int size)
    {
        int to_copy = std::min<int>(CONST_SIZE, size);
        int to_move = size >= CONST_SIZE ? 0 : CONST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float_slots[size], sample_buffer_complex_float_slots, to_move * sizeof(complex_t));

        std::memcpy(sample_buffer_complex_float_slots, buffer, to_copy * sizeof(complex_t));
    }

    void ConstellationViewerDVBS2::pushComplexPilots(complex_t *buffer, int size)
    {
        has_pilots = true;

        int to_copy = std::min<int>(CONST_SIZE, size);
        int to_move = size >= CONST_SIZE ? 0 : CONST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float_pilots[size], sample_buffer_complex_float_pilots, to_move * sizeof(complex_t));

        std::memcpy(sample_buffer_complex_float_pilots, buffer, to_copy * sizeof(complex_t));
    }

    void ConstellationViewerDVBS2::draw()
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImVec2 rect_min = ImGui::GetCursorScreenPos();
        ImVec2 rect_max = { rect_min.x + d_constellation_size * ui_scale, rect_min.y + d_constellation_size * ui_scale };
        draw_list->AddRectFilled(rect_min, rect_max, style::theme.widget_bg);
        draw_list->PushClipRect(rect_min, rect_max);

        // Draw PLHeader
        for (int i = 0; i < CONST_SIZE / 4; i++)
        {
            draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + ((d_constellation_size / 2) * ui_scale + sample_buffer_complex_float_plheader[i].real * (d_constellation_size / 2) * d_hscale * ui_scale),
                                              ImGui::GetCursorScreenPos().y + ((d_constellation_size / 2) * ui_scale + sample_buffer_complex_float_plheader[i].imag * (d_constellation_size / 2) * d_vscale * ui_scale)),
                                       2 * ui_scale * (d_constellation_size / 200.0f),
                                       style::theme.red);
        }

        // Draw Slots
        for (int i = 0; i < CONST_SIZE; i++)
        {
            draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + ((d_constellation_size / 2) * ui_scale + sample_buffer_complex_float_slots[i].real * (d_constellation_size / 2) * d_hscale * ui_scale),
                                              ImGui::GetCursorScreenPos().y + ((d_constellation_size / 2) * ui_scale + sample_buffer_complex_float_slots[i].imag * (d_constellation_size / 2) * d_vscale * ui_scale)),
                                       2 * ui_scale * (d_constellation_size / 200.0f),
                                       style::theme.constellation);
        }

        if (has_pilots)
        {
            // Draw Pilots
            for (int i = 0; i < CONST_SIZE; i++)
            {
                draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + ((d_constellation_size / 2) * ui_scale + sample_buffer_complex_float_pilots[i].real * (d_constellation_size / 2) * d_hscale * ui_scale),
                                                  ImGui::GetCursorScreenPos().y + ((d_constellation_size / 2) * ui_scale + sample_buffer_complex_float_pilots[i].imag * (d_constellation_size / 2) * d_vscale * ui_scale)),
                                           2 * ui_scale * (d_constellation_size / 200.0f),
                                           style::theme.red);
            }
        }

        draw_list->PopClipRect();
        ImGui::Dummy(ImVec2(d_constellation_size * ui_scale + 3, d_constellation_size * ui_scale + 3));
    }
}