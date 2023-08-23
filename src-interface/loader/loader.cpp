#include "imgui/imgui.h"
#include "imgui/imgui_flags.h"
#include "core/style.h"
#include "loader.h"

namespace satdump
{
    void draw_loader(int width, int height, float scale, GLuint *image_texture, std::string str)
    {
    	const std::string title = "SatDump";
    	const std::string slogan = "General Purpose Satellite Data Processor";
    	
    	ImGui::NewFrame();
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({(float)width, (float)height});
        ImGui::Begin("Loading Screen", nullptr, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);

        if(width > height)
        {
            ImVec2 reference_pos = { (float)width * 0.2f, ((float)height * 0.5f) - (125 * scale)};
            ImGui::SetCursorPos(reference_pos);
            ImGui::Image((void*)(intptr_t)(*image_texture), ImVec2(200 * scale, 200 * scale));
            ImGui::SetCursorPos({ reference_pos.x + (230 * scale), reference_pos.y + (40 * scale) });
            ImGui::PushFont(style::bigFont);
            ImGui::TextUnformatted(title.c_str());
            ImGui::PopFont();
            ImGui::SetCursorPos({ reference_pos.x + (230 * scale), reference_pos.y + (87 * scale) });
            ImGui::TextUnformatted(slogan.c_str());
            ImGui::GetWindowDrawList()->AddLine({reference_pos.x + (230 * scale), reference_pos.y + (112 * scale)}, {reference_pos.x + (490 * scale), reference_pos.y + (112 * scale)}, IM_COL32(155, 155, 155, 255));
            ImGui::SetCursorPos({ reference_pos.x + (230 * scale), reference_pos.y + (120 * scale) });
        }
        else
        {
            ImGui::PushFont(style::bigFont);
            ImVec2 title_size = ImGui::CalcTextSize(title.c_str());
            ImGui::SetCursorPos({((float)width / 2) - (150 * scale), ((float)height / 2) - title_size.y - (315 * scale)});
            ImGui::Image((void*)(intptr_t)(*image_texture), ImVec2(300 * scale, 300 * scale));
            ImGui::SetCursorPos({((float)width / 2) - (title_size.x / 2), ((float)height / 2) - title_size.y});
            ImGui::TextUnformatted(title.c_str());
            ImGui::PopFont();
            ImVec2 slogan_size = ImGui::CalcTextSize(slogan.c_str());
            ImGui::SetCursorPos({ ((float)width / 2) - (slogan_size.x / 2), ((float)height / 2) + (10 * scale) });
            ImGui::TextUnformatted(slogan.c_str());
            ImGui::GetWindowDrawList()->AddLine({((float)width / 2) - (slogan_size.x / 2), ((float)height / 2) + (20 * scale) + slogan_size.y}, 
                {((float)width / 2) + (slogan_size.x / 2), ((float)height / 2) + (20 * scale) + slogan_size.y}, IM_COL32(155, 155, 155, 255));
            ImGui::SetCursorPos({((float)width / 2) - (ImGui::CalcTextSize(str.c_str()).x / 2), ((float)height / 2) + (25 * scale) + slogan_size.y});
        }

        ImGui::TextDisabled("%s", str.c_str());
        ImGui::End();
        ImGui::Render();
    }
}
